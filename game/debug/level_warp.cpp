#include "debug/level_warp.h"

#include "core.h"
#include "execution/guest_turns.h"
#include "title/native_ownership.h"
#include "video/scene_cut.h"

#include <lucent/log.h>

namespace ctr {

bool LevelWarp::legal(Core &core) {
  const SceneIdentity scene = SceneCut::read(core);
  return scene.mainState == native::kMainStateRunning && !scene.loading &&
         scene.levelId <= native::kLastRaceTrackLevel &&
         core.mem_r32(core.r[28] + native::kLoadRequestGpOffset) == native::kNoLoadRequest;
}

std::string LevelWarp::arm(uint32_t level) {
  armed_ = level;
  return lucent::format("ok: warp armed for level 0x{:X}", level);
}

std::optional<uint32_t> LevelWarp::apply(Core &core) {
  if (!armed_ || !legal(core)) {
    return std::nullopt;
  }
  const uint32_t level = *armed_;
  armed_.reset();
  // The field resumes the main loop with the registers it stopped with.
  const R3000 resume = core;
  core.r[psx::cpu::kA0] = level;
  core.r[psx::cpu::kRa] = native::kFrameLoopResume;
  guest::callToReturn(core, native::kRequestLevelLoad, "CTR level warp");
  static_cast<R3000 &>(core) = resume;
  lucent::info("ctr-warp", "level 0x{:X} requested", level);
  return level;
}

} // namespace ctr
