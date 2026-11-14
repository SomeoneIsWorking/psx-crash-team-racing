#pragma once

#include "boot/startup_audio_wait.h"
#include "boot/startup_resource_load.h"
#include "boot/startup_resource_pump.h"
#include "debug/level_warp.h"
#include "disc/async_disc_owner.h"
#include "execution/spu_dma_callback_registration.h"
#include "execution_exit.h"
#include "frame/field_boundary.h"
#include "frame/frame_callback_owner.h"
#include "frame/frame_suffix.h"
#include "game_runtime.h"
#include "video/presentation_owner.h"
#include "video/projection_owner.h"
#include "video/widescreen_owner.h"

#include <cstdint>
#include <optional>
#include <string>

class Core;
class Game;

namespace ctr {

// Owns one CTR state-3 frame per step (the suffix's DrawOTag, then the next frame's logic and draw to its wait) and
// picks the continuation owed when it enters. The first field
// enters the executable at the PC the admitted header set.
class FrameDriver final : public psx::game::FrameDriver {
public:
  explicit FrameDriver(Game &game);

  void stepFrame(Core &core, uint32_t frame) override;

  [[nodiscard]] uint32_t completedFrames() const;
  [[nodiscard]] uint64_t budgetExitsForLastField() const;
  [[nodiscard]] const ProjectionOwner &projection() const;
  [[nodiscard]] const WidescreenOwner &widescreen() const;
  [[nodiscard]] const PresentationOwner &presentation() const;

  // Arms a track arrival; the next field raises it through the guest's own level-load request.
  std::string armWarp(uint32_t level);

private:
  // Guest overrides are bare function pointers, so each callsite needs its own entry point.
  static void onStartupGpuVSync(Core *core);
  static void onStartupDisplayVSync(Core *core);
  static void onBootResourceWait(Core *core);
  static void onBootResourcePump(Core *core);
  static void onStartupAudioService(Core *core);
  static void onShutdownVSync(Core *core);
  static void onFrameTiming(Core *core);
  static void onProjectionProducer(Core *core);
  static void onDmaCallbackRegistration(Core *core);

  // `dispatchField` is the only place a budget exit may be resumed as the same field; with `stop` it also ends where
  // execution reaches that address (RenderFrame's entry, where the draw replay samples).
  // The owed continuation; a guest turn it ran is the field's.
  std::optional<psx::cpu::ExecutionResult> resumeOwedContinuation(Core &core);
  // The frame driver's display field: the clock's advance with its VBlank and DrawSync callbacks, the pad and the SPU.
  void deliverFields(Core &core);
  // RenderFrame reached its suffix: fields until the suffix's wait holds no more, then the suffix and its DrawOTag.
  void closeFrame(Core &core);
  // Commits the fields the step delivered.
  void finishStep(Core &core);
  void refuseUnfinishedField(Core &core, uint32_t frame, const std::optional<psx::cpu::ExecutionResult> &execution);
  [[nodiscard]] psx::cpu::ExecutionResult
  dispatchField(Core &core, uint32_t entry, std::optional<uint32_t> stop = std::nullopt);
  void publishMeasuredProjection(Core &core);
  [[nodiscard]] static ProjectionOwner::Source classifyProjectionSource(uint32_t returnAddress);

  DiscReadOwner &discReadOwner_;
  FrameCallbackOwner frameCallbacks_;
  ProjectionOwner projection_;
  WidescreenOwner widescreen_;
  PresentationOwner presentation_;
  FieldBoundary field_;
  StartupResourceLoad resourceLoad_;
  StartupResourcePump resourcePump_;
  StartupAudioWait startupAudio_;
  SpuDmaCallbackRegistration spuDmaRegistration_;
  FrameSuffix frameSuffix_;
  LevelWarp warp_;
  uint64_t budgetExitsThisField_ = 0;
  static constexpr uint32_t kMaxFrameFields = 4u;
  // The fields this step delivered.
  uint32_t stepFields_ = 0;
  bool bootEntered_ = false;
  // The host loop's number for the previous field; the base is the loop's, consecutiveness is ours.
  std::optional<uint32_t> lastFrame_;
};

} // namespace ctr
