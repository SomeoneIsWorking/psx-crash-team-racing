#pragma once

#include "disc/overlay_image_owner.h"

#include <cstdint>
#include <optional>

class Core;

namespace ctr {

// Stock libcd ends CdRead from the CD interrupt via the callback at 0x800771B0 (slot 0x8008AD10);
// psxport transfers synchronously, so that completion is owed until the issuing call unwinds.
class DiscReadOwner final {
public:
  // Called from the CdRead leaf: the completion is owed from here until the calling loader returns.
  void noteTransferComplete(Core &core, std::optional<CompletedDiscRead> read = std::nullopt);

  void deliverPending(Core &core);

  // Mid-field delivery for a guest that polls for its completion, once the loader that called CdRead has
  // returned, so it has published its state first. Disc reads take no drive time.
  void deliverIfDue(Core &core);

  [[nodiscard]] bool hasPendingCompletion() const {
    return pending_;
  }
  [[nodiscard]] uint32_t deliveredCallbacks() const {
    return deliveredCallbacks_;
  }
  [[nodiscard]] uint32_t polledReads() const {
    return polledReads_;
  }

private:
  OverlayImageOwner overlayImages_;
  bool pending_ = false;
  uint32_t issuerStack_ = 0; // guest sp inside the function that called CdRead
  uint32_t deliveredCallbacks_ = 0;
  uint32_t polledReads_ = 0;
};

// The CdRead leaf: the stock synchronous transfer, then an owed completion callback.
void readWithCompletionCallback(Core *core);

} // namespace ctr
