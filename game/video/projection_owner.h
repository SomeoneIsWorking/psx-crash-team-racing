#pragma once

#include "guest_widescreen_projection.h"
#include "video/view_projection_plan.h"

#include <cstdint>
#include <functional>

class Core;

namespace ctr {

struct ProjectionPublication {
  uint32_t source = 0;
  int32_t nativeWidth = 0;
  int32_t nativeHeight = 0;
  int32_t centerX = 0;
  int32_t centerY = 0;
  uint32_t screenDistance = 0;
  uint64_t sequence = 0;

  [[nodiscard]] bool valid() const {
    return source != 0 && nativeWidth > 0 && nativeHeight > 0 && screenDistance > 0;
  }
};

// Captures the view input of the retail projection publication, refuses if the published libgte
// state disagrees, then resolves the widening plan once from that view.
class ProjectionOwner final {
public:
  using RetailBody = std::function<void(Core &)>;

  // Identified by the return address that reached the owner.
  enum class Source : uint8_t {
    LensFlare = 0, // 0x80024CCC, inside the lens-flare producer [0x80024C4C,0x80025138)
    StateZero = 1, // 0x8003BD2C
    Overlay = 2,   // 0x8003F5C0
  };
  [[nodiscard]] static uint32_t sourceReturnAddress(Source source);

  void publish(Core &core, const RetailBody &retailBody, Source source);

  [[nodiscard]] const ProjectionPublication &previous() const;
  [[nodiscard]] const ProjectionPublication &current() const;

  // The plan is derived from the guest's own view on every publication: the boot display mode is not
  // the projected extent (512 wide, OFX = width/2).
  using PlanSource = std::function<psx::present::GuestProjectionPlan(Core &, const GuestViewProjection &)>;
  void setPlanSource(PlanSource source);

  // Read by `ViewFrustumOwner`, so the plan has one home.
  [[nodiscard]] const psx::present::GuestProjectionPlan &plan() const {
    return plan_;
  }

private:
  ProjectionPublication previous_{};
  ProjectionPublication current_{};
  psx::present::GuestProjectionPlan plan_{};
  bool planLatched_ = false;
  PlanSource planSource_{};
};

} // namespace ctr
