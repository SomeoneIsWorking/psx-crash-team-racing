#include "debug/debug_menu.h"

#include "core.h"
#include "debug/level_warp.h"
#include "frame/frame_driver.h"
#include "game.h"
#include "title/native_ownership.h"

#include <lucent/log.h>

#include <vector>

namespace ctr {

DebugMenu::DebugMenu() {
  warp({
      .levels = {"track"},
      .places =
          [](Core &) {
            std::vector<psx::game::WarpPlace> tracks;
            for (std::uint32_t level = 0; level <= native::kLastRaceTrackLevel; ++level) {
              tracks.push_back({
                  .name = lucent::format("track 0x{:02X}", level),
                  .arrive =
                      [level](Core &core) {
                        return core.game->frameDriverAs<FrameDriver>().armWarp(level);
                      },
              });
            }
            return tracks;
          },
      .refusal =
          [](Core &core) {
            return core.game->frameDriverIf<FrameDriver>() != nullptr && LevelWarp::legal(core)
                       ? std::string()
                       : "a warp is legal only during a race with no load pending";
          },
  });
}

} // namespace ctr
