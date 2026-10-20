#pragma once

#include "guest_widescreen_projection.h"
#include "video/view_projection_plan.h"

#include <cstdint>

class Core;

namespace ctr {

class ProjectionOwner;

// Widening reaches H (GTE CR26), not the extent alone, because sx = H*X/Z + OFX; four
// `cfc2 rX,$26` sites derive 2H, 4H and H/2, so the guest's viewport math moves with it.
class WidescreenOwner final {
public:
  struct LatchResult {
    bool widens = false;
    psx::present::GuestProjectionPlan plan{};
  };

  psx::present::GuestProjectionPlan planFor(Core &core, const GuestViewProjection &view);

  // Resolved on the first publication, not at boot.
  [[nodiscard]] bool latched() const;

  // The resolved plan lives in the projection owner.
  void report(const GuestViewProjection &view) const;

  static const char *aspectName(psx::present::PresentationAspect aspect);

private:
  psx::present::GuestProjectionPlan plan_{};
  int32_t distanceScaleNumerator_ = 1;
  int32_t distanceScaleDenominator_ = 1;
  bool latched_ = false;
};

} // namespace ctr
