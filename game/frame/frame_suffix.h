#pragma once

#include <cstdint>

class Core;

namespace ctr {

// The three guest words the retail frame suffix at 0x80037880 waits on.
struct FrameSuffixWait {
  // Zero means none is published, which is not a pending draw.
  uint32_t gameState = 0;
  // Nonzero while the published draw is outstanding.
  bool drawSyncPending = false;
  bool vblankFieldsPending = false;
  uint32_t registeredCallbacks = 0;
};

[[nodiscard]] bool frameSuffixStillWaiting(const FrameSuffixWait &wait);

// The frame-timing bridge and the retail suffix that closes the frame. Its wait is the frame driver's: the driver
// delivers fields while `waiting`, then `finish` runs the suffix through DrawOTag back to the main loop.
class FrameSuffix final {
public:
  [[nodiscard]] bool isPending() const;

  // The timing leaf's conditional debug VSync(0) is omitted.
  void completeFieldTiming(Core &core);
  [[nodiscard]] bool waiting(Core &core) const;
  void finish(Core &core);

private:
  [[nodiscard]] FrameSuffixWait readWaitState(Core &core) const;

  bool pending_ = false;
};

} // namespace ctr
