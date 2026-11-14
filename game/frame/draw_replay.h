#pragma once

#include "frame/draw_section.h"
#include "motion_set.h"
#include "replay_frame.h"

#include <array>
#include <bitset>
#include <cstdint>

class Core;

namespace ctr {

// The instances on JitPools.instance's taken list, for the record size of one player count.
class InstancePool final : public psx::present::MotionLiveness {
public:
  static constexpr std::uint32_t kMaxItems = 128u;

  explicit InstancePool(std::uint32_t itemSize) : itemSize_(itemSize) {}
  void sample(Core &core) override;
  [[nodiscard]] bool live(std::uint32_t record) const override;

private:
  std::uint32_t itemSize_;
  std::uint32_t base_ = 0;
  std::bitset<kMaxItems> taken_;
};

// CTR's in-betweens: RenderFrame's picture half replayed with the view, the instances' matrices and the drivers'
// shadow inputs blended.
class DrawReplay {
public:
  DrawReplay();

  [[nodiscard]] psx::present::ReplayFrame &frame() {
    return frame_;
  }

private:
  static constexpr std::size_t kPlayerCounts = 4;
  std::array<InstancePool, kPlayerCounts> pools_;
  std::array<psx::present::MotionTrack, 1 + kPlayerCounts + 8> tracks_;
  DrawSection section_;
  psx::present::ReplayFrame frame_;
};

} // namespace ctr
