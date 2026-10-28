#pragma once

#include "guest_widescreen_projection.h"
#include "video/view_projection_plan.h"

#include <cstdint>

class Core;

namespace ctr {

class ProjectionOwner;

// The widening plan: the guest keeps its H and centre, the canvas gains a margin each side and the cull
// (`ViewFrustumOwner`) widens to match, so the extra field is rendered, not zoomed.
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
  bool latched_ = false;
};

} // namespace ctr
