#include "boot/startup_audio_wait.h"

#include "core.h"
#include "execution/guest_turns.h"
#include "execution_exit.h"
#include "frame/field_boundary.h"
#include "title/native_ownership.h"

#include <cstdlib>
#include <lucent/log.h>
#include <utility>

namespace ctr {

StartupAudioWait::StartupAudioWait(FieldBoundary &field) : field_(field) {}

bool StartupAudioWait::ownsContinuation() const {
  return continuationAddress_ != 0;
}

void StartupAudioWait::service(Core &core) {
  const uint32_t caller = core.r[31];
  guest::callOriginalToReturn(core, native::kStartupAudioService, "CTR startup-audio service");
  if (caller != native::kStartupAudioServiceReturn) {
    return;
  }
  if (core.mem_r32(native::kStartupAudioWaitState) == 0u) {
    return;
  }
  if (ownsContinuation()) {
    lucent::error("ctr-frame", "nested state-zero audio waits are not supported");
    std::abort();
  }
  continuationAddress_ = native::kStartupAudioServiceReturn;
  field_.request(core);
}

uint32_t StartupAudioWait::takeContinuation() {
  return std::exchange(continuationAddress_, 0u);
}

} // namespace ctr
