#pragma once

#include "frame/field_override_scope.h"

#include <span>

namespace ctr {

// The race HUD at the screen's edges: each element's packets, wholly left or right of the centre column, move to that
// edge of the drawn picture (psxport anchorHudElement); one across the centre stays. The elements are
// UI_RenderFrame_Racing's drawers, and the text and GT4 calls it makes itself (the big position number, the turbo
// count). Guest RAM keeps the authored 4:3 layout.
class HudAnchor final {
public:
  // The element drawers the frame driver owns for each field.
  [[nodiscard]] static std::span<const FieldOverrideBinding> bindings();
};

} // namespace ctr
