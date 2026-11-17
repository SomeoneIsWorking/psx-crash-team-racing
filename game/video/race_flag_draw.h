#pragma once

#include <cstdint>

class Core;

namespace ctr {

// RaceFlag_DrawSelf 0x800444E8, the checkered loading flag: retail's grid, wave, colours, packets and OT link, with
// the per-quad reject widened to the columns the presentation plan draws, and the grid continued past its last column
// while only a margin shows it, so a wide canvas keeps the flag across the margins.
class RaceFlagDraw final {
public:
  void draw(Core &core) const;
};

} // namespace ctr
