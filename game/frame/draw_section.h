#pragma once

#include "replay_frame.h"

#include <array>
#include <cstdint>
#include <optional>

class Core;

namespace ctr {

// MainFrame_RenderFrame's picture half, up to its frame-timing call; the ordering table it links into is the one
// DrawOTag sends.
class DrawSection final : public psx::present::ReplaySection {
public:
  [[nodiscard]] std::optional<std::uint32_t> draw(Core &core, psx::present::GuestRedraw &redraw) const override;
  [[nodiscard]] bool linked(std::uint32_t head) const override;

private:
  // The two frame buffers' ordering-table heads, as the section last linked them.
  mutable std::array<std::uint32_t, 2> heads_{};
};

} // namespace ctr
