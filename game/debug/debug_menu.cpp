#include "debug/debug_menu.h"

#include "core.h"
#include "debug/level_warp.h"
#include "frame/frame_driver.h"
#include "game.h"
#include "title/native_ownership.h"

namespace ctr {

DebugMenu::DebugMenu() {
  using psx::game::DebugCall;
  using psx::game::DebugParam;
  section("Track Warp");
  verb({
      .name = "warp",
      .summary = "Load that race track",
      .params = {DebugParam::hex("level", 0, native::kLastRaceTrackLevel)},
      .run =
          [](const DebugCall &call) {
            call.reply = call.core.game->frameDriverAs<FrameDriver>().armWarp(call.values.u32(0));
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
