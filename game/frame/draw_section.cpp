#include "frame/draw_section.h"

#include "core.h"
#include "guest_redraw.h"
#include "title/native_ownership.h"

#include <algorithm>

namespace ctr {

std::optional<std::uint32_t> DrawSection::draw(Core &core, psx::present::GuestRedraw &redraw) const {
  const std::uint32_t tracker = core.mem_r32(native::kGameTrackerPointer);
  const std::array<std::uint32_t, 2> arguments{tracker, core.mem_r32(native::kGamepadsPointer)};
  // The card poll's events belong to the real frame.
  const std::array<psx::present::GuestRedraw::Skip, 1> skips{
      {{.at = native::kFrameCardPoll, .resume = native::kFrameCardPollEnd}}};
  if (!redraw.dispatchTo(native::kFrameOwner, native::kFrameDrawEnd, arguments, skips)) {
    return std::nullopt;
  }
  const std::uint32_t head =
      core.mem_r32(tracker + native::kTrackerOrderingTableOffset) + native::kOrderingTableHeadOffset;
  if (std::ranges::find(heads_, head) == heads_.end()) {
    heads_ = {heads_[1], head};
  }
  return head;
}

bool DrawSection::linked(std::uint32_t head) const {
  return head != 0u && std::ranges::find(heads_, head) != heads_.end();
}

} // namespace ctr
