#include "video/projection_owner.h"

#include "core.h"
#include "proj_params.h"
#include "title/native_ownership.h"

#include <cmath>
#include <cstdlib>
#include <lucent/log.h>

namespace ctr {
namespace {

int32_t signedHalf(uint16_t value) {
  return static_cast<int16_t>(value) / 2;
}

bool agrees(float actual, int64_t expected) {
  return std::fabs(actual - static_cast<float>(expected)) < 0.001f;
}

} // namespace

void ProjectionOwner::publish(Core &core, const RetailBody &retailBody, Source source) {
  if (!retailBody) {
    lucent::error("ctr-projection", "projection publication requires the preserved retail body");
    std::abort();
  }

  ProjectionPublication next{
      .source = core.r[4],
      .nativeWidth = static_cast<int16_t>(core.mem_r16(core.r[4] + native::kViewRectWidthOffset)),
      .nativeHeight = static_cast<int16_t>(core.mem_r16(core.r[4] + native::kViewRectHeightOffset)),
      .centerX = signedHalf(core.mem_r16(core.r[4] + native::kViewRectWidthOffset)),
      .centerY = signedHalf(core.mem_r16(core.r[4] + native::kViewRectHeightOffset)),
      .screenDistance = core.mem_r32(core.r[4] + native::kViewScreenDistanceOffset),
      .sequence = current_.sequence + 1u,
  };

  retailBody(core);

  const auto &published = core.rsub.projParams;
  if (!published.geomValid() || !agrees(published.geomOfx(), next.centerX) ||
      !agrees(published.geomOfy(), next.centerY) || !agrees(published.geomH(), next.screenDistance)) {
    lucent::error("ctr-projection",
                  "retail projection publication disagreed with view 0x{:08X}: expected ({},{},H={}), got "
                  "({},{},H={})",
                  next.source,
                  next.centerX,
                  next.centerY,
                  next.screenDistance,
                  published.geomOfx(),
                  published.geomOfy(),
                  published.geomH());
    std::abort();
  }

  previous_ = current_;
  current_ = next;
  if (planSource_ && !planLatched_) {
    // The guest's own H stands: the margins are drawn by widening the canvas, not by scaling H.
    planSource_(core,
                GuestViewProjection{.width = next.nativeWidth,
                                    .height = next.nativeHeight,
                                    .distance = static_cast<int32_t>(next.screenDistance)});
    planLatched_ = true;
  }
}

void ProjectionOwner::setPlanSource(PlanSource source) {
  planSource_ = std::move(source);
}

uint32_t ProjectionOwner::sourceReturnAddress(Source source) {
  switch (source) {
  case Source::LensFlare:
    return native::kProjectionReturnLensflare;
  case Source::StateZero:
    return native::kProjectionReturnState;
  case Source::Overlay:
    return native::kProjectionReturnOverlay;
  }
  return 0u;
}

const ProjectionPublication &ProjectionOwner::previous() const {
  return previous_;
}

const ProjectionPublication &ProjectionOwner::current() const {
  return current_;
}

} // namespace ctr
