#include "frame/draw_replay.h"

#include "core.h"
#include "title/native_ownership.h"

#include <algorithm>
#include <span>

namespace ctr {
namespace {

using psx::present::blendedWord;
using psx::present::Motion;
using psx::present::MotionField;
using psx::present::MotionKey;
using psx::present::MotionTrack;

// Layouts from external/ctr-native (include/namespace_PushBuffer.h, namespace_Instance.h, namespace_JitPool.h,
// namespace_Vehicle.h).
constexpr std::uint32_t kPoolTakenFirst = 0x0Cu;
constexpr std::uint32_t kPoolMaxItems = 0x18u;
constexpr std::uint32_t kPoolItemSize = 0x1Cu;
constexpr std::uint32_t kPoolData = 0x24u;
constexpr std::uint32_t kInstancePool = native::kGameTracker + native::kTrackerInstancePoolOffset;
// An instance is 0x74 bytes and one 0x88-byte draw record per player.
constexpr std::uint32_t kInstanceBytes = 0x74u;
constexpr std::uint32_t kInstancePerPlayerBytes = 0x88u;

// PushBuffer pos (x, y, z) then rot (x, y, z), 4096 a turn: the third halfword is z, the fourth rot.x.
constexpr std::array<MotionField, 3> kViewFields{{
    {.offset = 0x0u, .motion = Motion::Halves},
    {.offset = 0x4u, .motion = Motion::Halves},
    {.offset = 0x8u, .motion = Motion::Angle12x2},
}};

class ViewPose final : public psx::present::MotionRule {
public:
  void blend(const Samples &samples, float t, std::span<std::uint32_t> out) const override {
    out[0] = blendedWord(Motion::Halves, samples.from[0], samples.to[0], t);
    out[1] = (blendedWord(Motion::Halves, samples.from[1], samples.to[1], t) & 0xFFFFu) |
             (blendedWord(Motion::Angle12x2, samples.from[1], samples.to[1], t) & 0xFFFF0000u);
    out[2] = blendedWord(Motion::Angle12x2, samples.from[2], samples.to[2], t);
  }
};
const ViewPose kViewPose;

// Instance matrix (+0x30), the drawn transform; scale is apart at +0x1C.
constexpr auto kInstanceFields = psx::present::transformFields(0x30u);
const psx::present::TransformRule kInstanceTransform{psx::present::TransformRule::Rows::Scaled};
// The model and the owning thread: a slot freed and born again as another object does not pair.
constexpr std::array<MotionKey, 2> kInstanceKeys{{{.offset = 0x18u}, {.offset = 0x6Cu}}};

// Driver quadBlockHeight, posCurr, rotCurr (x, y, z, w) and AxisAngle3_normalVec: VehGroundShadow_Main's inputs.
constexpr std::array<MotionField, 8> kDriverFields{{
    {.offset = 0x2D0u, .motion = Motion::Linear},
    {.offset = 0x2D4u, .motion = Motion::Linear},
    {.offset = 0x2D8u, .motion = Motion::Linear},
    {.offset = 0x2DCu, .motion = Motion::Linear},
    {.offset = 0x2ECu, .motion = Motion::Angle12x2},
    {.offset = 0x2F0u, .motion = Motion::Angle12Low},
    {.offset = 0x370u, .motion = Motion::Halves},
    {.offset = 0x374u, .motion = Motion::Low16},
}};
// instSelf.
constexpr std::array<MotionKey, 1> kDriverKeys{{{.offset = 0x1Cu}}};

constexpr std::uint32_t instanceSize(std::size_t players) {
  return kInstanceBytes + kInstancePerPlayerBytes * static_cast<std::uint32_t>(players + 1);
}

} // namespace

void InstancePool::sample(Core &core) {
  taken_.reset();
  base_ = core.mem_r32(kInstancePool + kPoolData);
  if (core.mem_r32(kInstancePool + kPoolItemSize) != itemSize_) {
    return;
  }
  const std::uint32_t maxItems = std::min(core.mem_r32(kInstancePool + kPoolMaxItems), kMaxItems);
  std::uint32_t item = core.mem_r32(kInstancePool + kPoolTakenFirst);
  for (std::uint32_t walked = 0; item != 0u && walked < maxItems; ++walked) {
    const std::uint32_t offset = item - base_;
    if (offset % itemSize_ != 0u || offset / itemSize_ >= maxItems) {
      return;
    }
    taken_.set(offset / itemSize_);
    item = core.mem_r32(item);
  }
}

bool InstancePool::live(std::uint32_t record) const {
  const std::uint32_t offset = record - base_;
  return offset % itemSize_ == 0u && offset / itemSize_ < kMaxItems && taken_.test(offset / itemSize_);
}

DrawReplay::DrawReplay()
    : pools_{InstancePool{instanceSize(0)},
             InstancePool{instanceSize(1)},
             InstancePool{instanceSize(2)},
             InstancePool{instanceSize(3)}},
      frame_(section_, tracks_) {
  tracks_[0] = {.name = "view",
                .address = native::kGameTracker + native::kTrackerPushBufferOffset,
                .fields = kViewFields,
                .rule = &kViewPose,
                .records = {.stride = native::kPushBufferStride, .count = native::kPushBufferCount}};
  for (std::size_t players = 0; players < kPlayerCounts; ++players) {
    tracks_[1 + players] = {.name = "instances",
                            .address = kInstancePool + kPoolData,
                            .fields = kInstanceFields,
                            .rule = &kInstanceTransform,
                            .keys = kInstanceKeys,
                            .records = {.throughPointer = true,
                                        .stride = instanceSize(players),
                                        .count = InstancePool::kMaxItems,
                                        .liveness = &pools_[players]}};
  }
  for (std::uint32_t driver = 0; driver < native::kDriverCount; ++driver) {
    tracks_[1 + kPlayerCounts + driver] = {.name = "drivers",
                                           .address =
                                               native::kGameTracker + native::kTrackerDriversOffset + 4u * driver,
                                           .fields = kDriverFields,
                                           .keys = kDriverKeys,
                                           .records = {.throughPointer = true}};
  }
}

} // namespace ctr
