#pragma once

#include <cstdint>

namespace ctr {

// The view facts the guest hands to its publication at 0x80042910: width +0x20, height +0x22, H +0x18.
struct GuestViewProjection {
  int32_t width = 0;
  int32_t height = 0;
  int32_t distance = 0;

  [[nodiscard]] bool valid() const {
    return width > 0 && height > 0 && distance > 0;
  }
};

} // namespace ctr
