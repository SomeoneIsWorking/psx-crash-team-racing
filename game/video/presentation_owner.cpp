#include "video/presentation_owner.h"

#include "core.h"
#include "game.h"
#include "video/scene_cut.h"

#include <cstdlib>
#include <lucent/log.h>

namespace ctr {

PresentationOwner::PresentationOwner(SceneCut &sceneCut) : sceneCut_(sceneCut) {}

void PresentationOwner::finishField(Core &core, uint32_t fields) {
  if (!core.game) {
    lucent::error("ctr-presentation", "frame fence requires a bound Game");
    std::abort();
  }

  sceneCut_.observe(SceneCut::read(core));
  const uint64_t before = core.game->presentation.fence();
  core.game->presentation.commit(&core, static_cast<int>(fields));
  if (core.game->presentation.fence() != before + 1u) {
    lucent::error("ctr-presentation", "framework did not advance exactly one frame fence");
    std::abort();
  }
  ++completedFences_;
}

uint64_t PresentationOwner::completedFences() const {
  return completedFences_;
}

} // namespace ctr
