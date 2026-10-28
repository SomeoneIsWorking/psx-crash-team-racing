#pragma once

#include "guest_widescreen_projection.h"

#include <cstdint>
#include <functional>

class Core;

namespace ctr {

// The retail frustum builder takes its horizontal extent from the view's rect width; a full-width view is
// built at the plan's projection width so the cull keeps what the margins show.
class ViewFrustumOwner final {
public:
  using RetailBody = std::function<void(Core &)>;

  // The width the frustum is built at for a view rect at `x` with `width`.
  [[nodiscard]] static int32_t frustumWidth(int32_t x, int32_t width, const psx::present::GuestProjectionPlan &plan);

  // Runs the retail builder on the view in $a0 with its rect width widened for the call only.
  void build(Core &core, const RetailBody &retailBody, const psx::present::GuestProjectionPlan &plan) const;
};

} // namespace ctr
