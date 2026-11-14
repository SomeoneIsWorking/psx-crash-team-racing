#include "frame/frame_driver.h"

#include "core.h"
#include "execution/guest_turns.h"
#include "execution_control.h"
#include "execution_exit.h"
#include "field_rate.h"
#include "frame/field_override_scope.h"
#include "frame/retail_return.h"
#include "frame/vsync_bridge.h"
#include "game.h"
#include "runtime/session.h"
#include "title/native_ownership.h"

#include <array>
#include <cstdlib>
#include <lucent/log.h>
#include <utility>

namespace ctr {
namespace {

// The three extracted VSync callsites; the shutdown wait passes 30 and the startup waits 0.
constexpr VsyncBridge kStartupGpuVSyncBridge{
    native::kStartupGpuInit, native::kStartupGpuInitReturn, native::kAfterFirstStartupVSync, 0u};
constexpr VsyncBridge kStartupDisplayVSyncBridge{
    native::kStartupDisplayInit, native::kStartupDisplayInitReturn, native::kAfterSecondStartupVSync, 0u};
constexpr VsyncBridge kShutdownVSyncBridge{
    native::kShutdownDisplay, native::kShutdownDisplayReturn, native::kAfterShutdownVSync, 30u};

} // namespace

FrameDriver::FrameDriver(Game &game)
    : discReadOwner_(session(game.core).discRead), presentation_(session(game.core).sceneCut), field_(presentation_),
      resourceLoad_(field_), resourcePump_(field_), startupAudio_(field_) {}

void FrameDriver::stepFrame(Core &core, uint32_t frame) {
  if (!core.game) {
    lucent::error("ctr-frame", "frame {} has no bound Game", frame);
    std::abort();
  }
  if (lastFrame_ && frame != *lastFrame_ + 1u) {
    lucent::error("ctr-frame", "non-sequential frame {} requested after frame {}", frame, *lastFrame_);
    std::abort();
  }
  lastFrame_ = frame;
  budgetExitsThisField_ = 0;
  // The widening plan is resolved from the guest's own published view in ProjectionOwner::publish.
  projection_.setPlanSource([this](Core &projectionCore, const GuestViewProjection &view) {
    widescreen_.latchPlan(projectionCore, view);
  });

  const std::array<FieldOverrideBinding, 9> fieldOverrides{{
      {native::kStartupGpuInit, "startup GPU VSync owner", onStartupGpuVSync},
      {native::kStartupDisplayInit, "startup display VSync owner", onStartupDisplayVSync},
      {native::kBootResourceWait, "boot resource wait owner", onBootResourceWait},
      {native::kBootResourcePump, "boot resource pump owner", onBootResourcePump},
      {native::kStartupAudioService, "startup audio wait owner", onStartupAudioService},
      {native::kShutdownDisplay, "shutdown VSync owner", onShutdownVSync},
      {native::kFrameTiming, "frame timing owner", onFrameTiming},
      {native::kProjectionProducer, "projection owner", onProjectionProducer},
      {native::kDmaCallback, "DMA callback registration owner", onDmaCallbackRegistration},
  }};
  const FieldOverrideScope overrides(core, fieldOverrides);
  field_.beginField();

  core.game->timing.logicFrame = frame;
  core.rsub.otAttr.beginLogicFrame(frame);
  core.game->timing.frameTick();
  // Retail delivers this from the CD interrupt, and the loader stores its buffer only after the read returns.
  discReadOwner_.deliverPending(core);

  std::optional<psx::cpu::ExecutionResult> execution;
  if (!field_.pending(core)) {
    execution = resumeOwedContinuation(core);
  }
  if (field_.pending(core) || (execution && execution->reason == psx::cpu::ExecutionExitReason::FrameBoundary)) {
    finishStep(core);
    return;
  }
  if (execution) {
    refuseUnfinishedField(core, frame, execution);
  }

  if (resourceLoad_.ownsSuffix()) {
    resourceLoad_.resume(core);
  } else if (!bootEntered_) {
    bootEntered_ = true;
    execution = dispatchField(core, core.pc);
  } else {
    warp_.apply(core);
    execution = dispatchField(core, native::kFrameLoopResume, native::kFrameOwner);
    if (execution->reason == psx::cpu::ExecutionExitReason::GuestReturn) {
      session(core).drawReplay.frame().beforeDraw(core);
      execution = dispatchField(core, native::kFrameOwner);
    }
  }
  if (field_.pending(core) || (execution && execution->reason == psx::cpu::ExecutionExitReason::FrameBoundary)) {
    finishStep(core);
    return;
  }

  refuseUnfinishedField(core, frame, execution);
}

// The continuation ladder; only one continuation can be owed at a time.
std::optional<psx::cpu::ExecutionResult> FrameDriver::resumeOwedContinuation(Core &core) {
  if (startupAudio_.ownsContinuation()) {
    return dispatchField(core, startupAudio_.takeContinuation());
  }
  if (resourcePump_.isActive()) {
    resourcePump_.resume(core);
  } else if (resourceLoad_.consumeWaitedField()) {
    field_.request(core);
  }
  return std::nullopt;
}

void FrameDriver::deliverFields(Core &core) {
  // A field's wait spans the field edges the guest clock already ran past.
  const uint32_t fields = core.game->timing.vsyncFields(1u);
  for (uint32_t field = 0; field < fields; ++field) {
    if (!core.game->timing.advanceDisplayFields({1, 1},
                                                psx::frame::fieldRateMilliHz(core.game->timing.displayFieldRate()))) {
      lucent::error("ctr-frame", "the display clock refused a field");
      std::abort();
    }
    // The VBlank callback the advance raised, and a DrawSync callback still owed.
    frameCallbacks_.deliverField(core);
    // libpad reads the pad each VBlank; XA playback is pull-driven (state zero waits on the clip task at 0x8008D708).
    core.game->pad.serviceFrame();
    core.game->spu_audio.frame();
  }
  stepFields_ += fields;
}

void FrameDriver::closeFrame(Core &core) {
  // The suffix waits for the DrawSync callback and the VBlank callback's two-field countdown [gp+0x348].
  while (frameSuffix_.waiting(core)) {
    if (stepFields_ >= kMaxFrameFields) {
      const uint32_t state = core.mem_r32(core.r[28] + native::kGameStateGpOffset);
      lucent::error("ctr-frame",
                    "the frame suffix still waits after {} fields (countdown {}, DrawSync pending {}, pc 0x{:08X})",
                    stepFields_,
                    static_cast<int32_t>(core.mem_r32(core.r[28] + native::kFrameWaitFieldsGpOffset)),
                    core.mem_r8(state + native::kDrawSyncPendingOffset),
                    core.pc);
      std::abort();
    }
    deliverFields(core);
  }
  frameSuffix_.finish(core);
  // DrawOTag's draw completed synchronously, so its DrawSync callback is owed before the next frame runs.
  frameCallbacks_.deliverField(core);
}

void FrameDriver::finishStep(Core &core) {
  if (frameSuffix_.isPending()) {
    (void)core.executionControl().consume(core.pc, 0);
    closeFrame(core);
  }
  // A step that ended without the suffix's wait (boot, a load) yields one field on the guest clock.
  if (stepFields_ == 0u) {
    deliverFields(core);
  }
  field_.finishField(core, std::exchange(stepFields_, 0u));
}

void FrameDriver::refuseUnfinishedField(Core &core,
                                        uint32_t frame,
                                        const std::optional<psx::cpu::ExecutionResult> &execution) {
  if (execution) {
    lucent::error("ctr-frame",
                  "retail execution left frame {} at 0x{:08X} with {}: {}",
                  frame,
                  execution->guestPc,
                  psx::cpu::executionExitName(execution->reason),
                  execution->detail);
  } else {
    lucent::error("ctr-frame", "retail execution returned without completing CTR frame {}", frame);
  }
  std::abort();
}

psx::cpu::ExecutionResult FrameDriver::dispatchField(Core &core, uint32_t entry, std::optional<uint32_t> stop) {
  const auto run = [&](uint32_t pc) {
    return stop ? guest::dispatchHop(core, pc, *stop) : guest::dispatch(core, pc);
  };
  auto result = run(entry);
  while (result.reason == psx::cpu::ExecutionExitReason::BudgetExhausted) {
    // A field has no return boundary: a budget exit that consumed cycles is the same
    // field, one that consumed none made no progress and is refused.
    if (result.guestPc != core.pc || result.cycles == 0) {
      lucent::error("ctr-frame",
                    "frame {} cannot resume budget exit {} at 0x{:08X}: Core PC=0x{:08X}, cycles={}",
                    core.game->timing.logicFrame,
                    budgetExitsThisField_ + 1u,
                    result.guestPc,
                    core.pc,
                    result.cycles);
      std::abort();
    }
    ++budgetExitsThisField_;
    // Retail raises the CD interrupt asynchronously; a guest polling for it never reaches a field start.
    discReadOwner_.deliverIfDue(core);
    result = run(result.guestPc);
  }
  return result;
}

uint32_t FrameDriver::completedFrames() const {
  return field_.completedFields();
}

uint64_t FrameDriver::budgetExitsForLastField() const {
  return budgetExitsThisField_;
}

const ProjectionOwner &FrameDriver::projection() const {
  return projection_;
}

const WidescreenOwner &FrameDriver::widescreen() const {
  return widescreen_;
}

const PresentationOwner &FrameDriver::presentation() const {
  return presentation_;
}

std::string FrameDriver::armWarp(uint32_t level) {
  return warp_.arm(level);
}

void FrameDriver::onStartupGpuVSync(Core *core) {
  runVsyncBridge(*core, kStartupGpuVSyncBridge);
}

void FrameDriver::onStartupDisplayVSync(Core *core) {
  runVsyncBridge(*core, kStartupDisplayVSyncBridge);
}

void FrameDriver::onShutdownVSync(Core *core) {
  runVsyncBridge(*core, kShutdownVSyncBridge);
}

void FrameDriver::onBootResourceWait(Core *core) {
  core->game->frameDriverAs<FrameDriver>().resourceLoad_.begin(*core);
}

void FrameDriver::onBootResourcePump(Core *core) {
  core->game->frameDriverAs<FrameDriver>().resourcePump_.begin(*core);
}

void FrameDriver::onStartupAudioService(Core *core) {
  core->game->frameDriverAs<FrameDriver>().startupAudio_.service(*core);
}

void FrameDriver::onDmaCallbackRegistration(Core *core) {
  core->game->frameDriverAs<FrameDriver>().spuDmaRegistration_.service(*core);
}

void FrameDriver::onFrameTiming(Core *core) {
  FrameDriver &driver = core->game->frameDriverAs<FrameDriver>();
  if (core->r[31] == native::kFrameTimingReturn) {
    driver.frameSuffix_.completeFieldTiming(*core);
    // The suffix's wait takes interrupts, which no override can: the driver closes the frame once the turn ends.
    driver.field_.request(*core);
    return;
  }
  if (core->r[31] == native::kFrameTimingQueryReturn) {
    guest::callOriginalToReturn(*core, native::kFrameTiming, "CTR frame-timing query");
    return;
  }
  refuseUnexpectedRetailReturn("frame timing owner", native::kFrameTimingReturn, core->r[31]);
}

void FrameDriver::onProjectionProducer(Core *core) {
  core->game->frameDriverAs<FrameDriver>().publishMeasuredProjection(*core);
}

void FrameDriver::publishMeasuredProjection(Core &core) {
  const uint32_t returnAddress = core.r[31];
  const ProjectionOwner::Source source = classifyProjectionSource(returnAddress);
  projection_.publish(
      core,
      [](Core &projectionCore) {
        guest::callOriginalToReturn(projectionCore, native::kProjectionProducer, "CTR projection producer");
      },
      source);
}

ProjectionOwner::Source FrameDriver::classifyProjectionSource(uint32_t returnAddress) {
  // The three return addresses are the identity; this is the single list of them.
  if (returnAddress == native::kProjectionReturnLensflare) {
    return ProjectionOwner::Source::LensFlare;
  }
  if (returnAddress == native::kProjectionReturnState) {
    return ProjectionOwner::Source::StateZero;
  }
  if (returnAddress == native::kProjectionReturnOverlay) {
    return ProjectionOwner::Source::Overlay;
  }
  lucent::error("ctr-projection",
                "projection owner reached from unmeasured return 0x{:08X}; expected one of "
                "0x{:08X}/0x{:08X}/0x{:08X}",
                returnAddress,
                native::kProjectionReturnLensflare,
                native::kProjectionReturnState,
                native::kProjectionReturnOverlay);
  std::abort();
}

} // namespace ctr
