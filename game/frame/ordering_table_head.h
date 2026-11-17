#pragma once

#include "core.h"
#include "title/native_ownership.h"

#include <cstdint>

namespace ctr {

// The entry DrawOTag starts the frame's walk at: push buffer 0's table, last entry (ClearOTagR order). The HUD's
// table links in after the views', so this walk draws it too.
[[nodiscard]] inline uint32_t orderingTableHead(Core &core, uint32_t tracker) {
  return core.mem_r32(tracker + native::kTrackerOrderingTableOffset) + native::kOrderingTableHeadOffset;
}

} // namespace ctr
