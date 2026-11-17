#pragma once

#include "frame/field_override_scope.h"

#include <array>
#include <cstdint>
#include <span>

class Core;

namespace ctr {

// The race HUD at the screen's edges: each element's packets, wholly left or right of the centre column, move to that
// edge of the drawn picture (psxport anchorHudElement); one across the centre stays. The elements are
// UI_RenderFrame_Racing's drawers, and the text and GT4 calls it makes itself (the big position number, the turbo
// count). The rank suffix and the wumpa count each lead a group: the HUD instance beside it (the rank digit, the
// fruit), drawn later in the frame by RenderBucket_Execute, takes the same shift. Guest RAM keeps the authored 4:3
// layout.
class HudAnchor final {
public:
  // A 2D element and the HUD instance drawn beside it.
  enum class Group : uint8_t { Rank, Wumpa, Count };

  // The element drawers and the instance pass the frame driver owns for each field.
  [[nodiscard]] static std::span<const FieldOverrideBinding> bindings();

  // The columns `group`'s 2D element moved this frame.
  void shiftGroup(Group group, int dx);
  // RenderBucket_Execute: the guest's call, then each shifted group's instance packets moved with its element.
  void drawInstances(Core &core);

private:
  std::array<int, static_cast<std::size_t>(Group::Count)> shifts_{};
};

} // namespace ctr
