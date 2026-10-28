#include "video/view_frustum_owner.h"

#include "core.h"
#include "title/native_ownership.h"

#include <cstdlib>
#include <lucent/log.h>

namespace ctr {

int32_t ViewFrustumOwner::frustumWidth(int32_t x, int32_t width, const psx::present::GuestProjectionPlan &plan) {
  // Split-screen views are narrower than the native extent and keep their own frustum.
  const bool spansNativeExtent = x == 0 && width == plan.nativeProjectionExtent.width;
  if (!spansNativeExtent || plan.projectionExtent.width <= width) {
    return width;
  }
  return plan.projectionExtent.width;
}

void ViewFrustumOwner::build(Core &core,
                             const RetailBody &retailBody,
                             const psx::present::GuestProjectionPlan &plan) const {
  if (!retailBody) {
    lucent::error("ctr-frustum", "frustum build requires the preserved retail body");
    std::abort();
  }
  const uint32_t view = core.r[4];
  const auto x = static_cast<int16_t>(core.mem_r16(view + native::kViewRectXOffset));
  const auto width = static_cast<int16_t>(core.mem_r16(view + native::kViewRectWidthOffset));
  const int32_t built = frustumWidth(x, width, plan);
  if (built == width) {
    retailBody(core);
    return;
  }
  core.mem_w16(view + native::kViewRectWidthOffset, static_cast<uint16_t>(built));
  retailBody(core);
  core.mem_w16(view + native::kViewRectWidthOffset, static_cast<uint16_t>(width));
}

} // namespace ctr
