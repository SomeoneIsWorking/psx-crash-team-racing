#pragma once

#include <cstdint>

class Core;

namespace ctr {

class FieldBoundary;

// State zero's post-archive XA wait on the task at 0x8008D708 (the "SCEA presents" voice clip): serves the retail
// service call and yields one host field per call; the frame driver resumes the field at the retail poll. Start or
// Cross ends the clip through the guest's own fade request.
class StartupAudioWait final {
public:
  StartupAudioWait(FieldBoundary &field);

  [[nodiscard]] bool ownsContinuation() const;

  void service(Core &core);

  // The retail poll the next field resumes at; the wait no longer owns it.
  [[nodiscard]] uint32_t takeContinuation();

private:
  FieldBoundary &field_;
  uint32_t continuationAddress_ = 0;
};

} // namespace ctr
