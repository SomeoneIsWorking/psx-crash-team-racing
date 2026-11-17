#include "video/race_flag_draw.h"

#include "core.h"
#include "emit_memory.h"
#include "execution/guest_turns.h"
#include "frame/ordering_table_head.h"
#include "game.h"
#include "gp0_primitive_decode.h"
#include "gte_registers.h"
#include "title/native_ownership.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace ctr {
namespace {

namespace gte = psx::gte;
using psx::present::EmitMemory;

constexpr int32_t kColumns = 35;
// Columns past retail's last, drawn only into a margin.
constexpr int32_t kMaxMarginColumns = 32;
constexpr int32_t kRows = 10;
constexpr int32_t kPointsPerRow = 3;
constexpr std::size_t kColumnPoints = static_cast<std::size_t>(kRows) * kPointsPerRow;
constexpr uint32_t kBankBytes = kColumnPoints * 4u;
constexpr int16_t kFirstColumnX = -0xBBE;
constexpr int16_t kColumnStep = 100;
constexpr std::array<int16_t, kPointsPerRow> kTopY{-910, -816, -722};
constexpr int16_t kRowStep = 0x11A;
constexpr int32_t kDepth = 0x280;
constexpr uint32_t kRippleStep = 300u;
constexpr uint32_t kColumnPhaseStep = 0x100u;
constexpr uint32_t kLightPhase = 0xC80u;
constexpr int32_t kShadeOne = 0x2000;
constexpr uint32_t kPolyBytes = 0x24u;
constexpr uint32_t kSignMask = 0x80008000u;
// The retail reject window, packed (y << 16 | x): 512 x 216.
constexpr uint32_t kScreenWidth = 512u;
constexpr uint32_t kScreenHeight = 216u;
constexpr uint32_t kOffsetX = 256u << 16;
constexpr uint32_t kOffsetY = 120u << 16;
constexpr uint32_t kScreenDistance = 256u;
constexpr uint8_t kPolyWords = 8u;
constexpr uint8_t kPolyG4 = 0x38u;
constexpr uint32_t kOtAddressMask = 0x00FFFFFFu;

// The trig table packs two 16-bit quarter-wave samples per word.
int32_t sine(const EmitMemory &mem, uint32_t angle) {
  uint32_t word = mem.mem_r32(native::kTrigTable + (angle & 0x3FFu) * 4u);
  if ((angle & 0x400u) == 0u) {
    word <<= 16;
  }
  int32_t value = static_cast<int32_t>(word) >> 16;
  if ((angle & 0x800u) != 0u) {
    value = -value;
  }
  return value;
}

struct Wave {
  uint32_t angle1 = 0;
  int32_t amplitude = 0;
  uint32_t angle2 = 0;
  int32_t speed = 0;
  int32_t phase = 0;
};

Wave loadWave(const EmitMemory &mem) {
  return {mem.mem_r32(native::kRaceFlagWave),
          static_cast<int32_t>(mem.mem_r32(native::kRaceFlagWave + 4u)),
          mem.mem_r32(native::kRaceFlagWave + 8u),
          static_cast<int32_t>(mem.mem_r32(native::kRaceFlagWave + 12u)),
          static_cast<int32_t>(mem.mem_r32(native::kRaceFlagWave + 16u))};
}

void storeWave(const EmitMemory &mem, const Wave &wave) {
  mem.mem_w32(native::kRaceFlagWave, wave.angle1);
  mem.mem_w32(native::kRaceFlagWave + 4u, static_cast<uint32_t>(wave.amplitude));
  mem.mem_w32(native::kRaceFlagWave + 8u, wave.angle2);
  mem.mem_w32(native::kRaceFlagWave + 12u, static_cast<uint32_t>(wave.speed));
  mem.mem_w32(native::kRaceFlagWave + 16u, static_cast<uint32_t>(wave.phase));
}

// Advances the phase and, when it wraps, re-rolls amplitude and speed; returns the phase's table angle.
uint32_t stepWave(const EmitMemory &mem, Wave &wave, int32_t advance) {
  wave.phase = static_cast<int32_t>(static_cast<uint32_t>(wave.phase) + static_cast<uint32_t>(advance));
  auto angle = static_cast<uint32_t>(wave.phase >> 5);
  if (angle >= 0x1000u) {
    wave.phase &= 0x1FFFF;
    angle = static_cast<uint32_t>(wave.phase >> 5);
    wave.angle1 += 0x200u;
    wave.amplitude = (((sine(mem, wave.angle1) + 0xFFF) * 32) >> 13) + 0x96;
    wave.angle2 += 200u;
    wave.speed = (((sine(mem, wave.angle2) + 0xFFF) * 64) >> 13) + 0xB4;
  }
  return angle;
}

struct Column {
  std::array<int16_t, kPointsPerRow> x{};
  std::array<int16_t, kPointsPerRow> y{};
  int32_t depth = 0;
};

// One RTPT row: each point's depth rippled by `ripple`, which advances per point.
void projectRow(Core &core, const EmitMemory &mem, const Column &column, uint32_t &ripple) {
  static constexpr std::array<uint32_t, kPointsPerRow> kXy{gte::kVxy0, gte::kVxy1, gte::kVxy2};
  static constexpr std::array<uint32_t, kPointsPerRow> kZ{gte::kVz0, gte::kVz1, gte::kVz2};
  for (int32_t point = 0; point < kPointsPerRow; ++point) {
    const auto z = static_cast<int16_t>(column.depth + (((sine(mem, ripple) + 0xFFF) * 32) >> 13));
    ripple += kRippleStep;
    core.gte().writeData(kXy[point],
                         static_cast<uint16_t>(column.x[point]) |
                             (static_cast<uint32_t>(static_cast<uint16_t>(column.y[point])) << 16));
    core.gte().writeData(kZ[point], static_cast<uint32_t>(static_cast<int32_t>(z)));
  }
  core.gte().execute(gte::kRtpt);
}

void storeRow(Core &core, const EmitMemory &mem, uint32_t address) {
  for (uint32_t point = 0; point < kPointsPerRow; ++point) {
    mem.storeVertex(address + point * 4u, core.gte().projected(point));
  }
}

void lowerRow(Column &column) {
  for (int16_t &y : column.y) {
    y = static_cast<int16_t>(y + kRowStep);
  }
}

uint32_t grey(uint32_t level) {
  level &= 0xFFu;
  return level | (level << 8) | (level << 16);
}

int32_t shade(int32_t light, int32_t lit, int32_t dark) {
  return (light * lit + (kShadeOne - light) * dark) >> 13;
}

// Retail's packed reject (every corner left of / above 0, or every corner at or past the far edge, the X borrow
// running into Y), applied with X measured from the plan's left drawn column.
class QuadReject {
public:
  explicit QuadReject(uint32_t margin) : margin_(margin), far_((kScreenHeight << 16) | (kScreenWidth + 2u * margin)) {}

  [[nodiscard]] bool outside(std::array<uint32_t, 4> corners) const {
    uint32_t before = kSignMask;
    uint32_t past = kSignMask;
    for (uint32_t &corner : corners) {
      corner = (corner & 0xFFFF0000u) | ((corner + margin_) & 0xFFFFu);
      before &= corner;
      past &= far_ - corner;
    }
    return before != 0u || past != 0u;
  }

  // A column past retail's last is drawn while it lies wholly beyond one side of retail's 4:3 window and part of it
  // still reaches into that side's margin; the flag's matrix decides which way its columns run.
  [[nodiscard]] bool marginShows(std::span<const uint32_t> column) const {
    if (margin_ == 0u) {
      return false;
    }
    const auto width = static_cast<int32_t>(kScreenWidth);
    const auto margin = static_cast<int32_t>(margin_);
    bool allLeft = true;
    bool allRight = true;
    bool inLeftMargin = false;
    bool inRightMargin = false;
    for (const uint32_t word : column) {
      const int32_t x = static_cast<int16_t>(word & 0xFFFFu);
      allLeft = allLeft && x < 0;
      allRight = allRight && x >= width;
      inLeftMargin = inLeftMargin || x >= -margin;
      inRightMargin = inRightMargin || x < width + margin;
    }
    return (allLeft && inLeftMargin) || (allRight && inRightMargin);
  }

private:
  uint32_t margin_;
  uint32_t far_;
};

using ColumnVertices = std::array<psx::gte::ProjectedVertex, kColumnPoints>;

std::array<uint32_t, kColumnPoints> wordsOf(const ColumnVertices &vertices) {
  std::array<uint32_t, kColumnPoints> words{};
  for (std::size_t point = 0; point < vertices.size(); ++point) {
    words[point] = vertices[point].word();
  }
  return words;
}

// One packet per quad that survives the reject: a G4 between two vertical neighbours of the previous column (lit by
// its light) and of this one, checkered by column and segment, linked at the head of the flag's OT entry. A quad retail
// would not draw (a margin column's, or one retail's 4:3 reject drops) takes no guest packet: it is a host packet drawn
// where its link would have put it, so the pool and the table hold what retail's would.
class QuadEmitter {
public:
  // The game tracker, the flag's ordering-table entry and the walk that draws it.
  struct Target {
    uint32_t tracker = 0;
    uint32_t ot = 0;
    uint32_t root = 0;
  };
  // Which quad: its column (the later of the two it joins) and its segment down that column.
  struct Place {
    int32_t column = 0;
    int32_t segment = 0;
  };
  // The light of the earlier column and of the later one.
  struct Lights {
    int32_t previous = 0;
    int32_t current = 0;
  };

  QuadEmitter(Core &core, const EmitMemory &mem, Target target, const QuadReject &reject)
      : core_(core), mem_(mem), target_(target), reject_(reject), retail_(0u) {}

  // False when the pool never held a packet: retail returns there.
  [[nodiscard]] bool emit(Place place, Lights lights, std::array<psx::gte::ProjectedVertex, 4> corners) {
    const std::array<uint32_t, 4> words{corners[0].word(), corners[1].word(), corners[2].word(), corners[3].word()};
    if (reject_.outside(words)) {
      return true;
    }
    const bool dark = (((place.column >> 2) + (place.segment >> 2)) & 1) != 0;
    const int32_t previousShade = dark ? shade(lights.previous, 105, 160) : shade(lights.previous, 130, 255);
    const int32_t shadeHere = dark ? shade(lights.current, 105, 160) : shade(lights.current, 130, 255);
    if (place.column <= 0 || place.column >= kColumns || retail_.outside(words)) {
      Quad quad;
      quad.store(0, grey(static_cast<uint32_t>(previousShade)) | (uint32_t{kPolyG4} << 24));
      quad.storeVertex(1, corners[0]);
      quad.store(2, grey(static_cast<uint32_t>(shadeHere)));
      quad.storeVertex(3, corners[2]);
      quad.store(4, grey(static_cast<uint32_t>(previousShade)));
      quad.storeVertex(5, corners[1]);
      quad.store(6, grey(static_cast<uint32_t>(shadeHere)));
      quad.storeVertex(7, corners[3]);
      pending_.push_back(quad);
      return true;
    }
    const uint32_t buffer = mem_.mem_r32(target_.tracker + native::kTrackerBackBufferOffset);
    const uint32_t cursor = mem_.mem_r32(buffer + native::kBufferPrimCursorOffset);
    if (mem_.mem_r32(buffer + native::kBufferPrimGuardOffset) >= cursor) {
      poly_ = cursor;
      mem_.mem_w32(buffer + native::kBufferPrimCursorOffset, cursor + kPolyBytes);
    }
    // Retail reuses the last packet once the pool is spent.
    if (poly_ == 0u) {
      return false;
    }
    mem_.storeVertex(poly_ + 0x08u, corners[0]);
    mem_.storeVertex(poly_ + 0x18u, corners[1]);
    mem_.storeVertex(poly_ + 0x10u, corners[2]);
    mem_.storeVertex(poly_ + 0x20u, corners[3]);
    mem_.mem_w32(poly_ + 0x04u, grey(static_cast<uint32_t>(previousShade)));
    mem_.mem_w32(poly_ + 0x14u, grey(static_cast<uint32_t>(previousShade)));
    mem_.mem_w8(poly_ + 0x03u, kPolyWords);
    mem_.mem_w8(poly_ + 0x07u, kPolyG4);
    mem_.mem_w32(poly_ + 0x0Cu, grey(static_cast<uint32_t>(shadeHere)));
    mem_.mem_w32(poly_ + 0x1Cu, grey(static_cast<uint32_t>(shadeHere)));
    mem_.mem_w32(poly_, (mem_.mem_r32(poly_) & ~kOtAddressMask) | (mem_.mem_r32(target_.ot) & kOtAddressMask));
    mem_.mem_w32(target_.ot, (mem_.mem_r32(target_.ot) & ~kOtAddressMask) | (poly_ & kOtAddressMask));
    insertPendingAfter(poly_);
    return true;
  }

  // The host quads after the last guest one are the entry's newest: drawn first, right after the entry.
  void finish() {
    insertPendingAfter(target_.ot);
  }

private:
  using Quad = psx::gpu::PacketBuffer<kPolyWords>;

  // The host quads emitted since the guest packet before `node` (in emission order) are drawn right after it, newest
  // first, as their links would have run.
  void insertPendingAfter(uint32_t node) {
    for (auto quad = pending_.rbegin(); quad != pending_.rend(); ++quad) {
      core_.hostPackets.insertAfter(core_, target_.root, node, quad->view(), native::kRaceFlagDraw);
    }
    pending_.clear();
  }

  Core &core_;
  const EmitMemory &mem_;
  Target target_;
  const QuadReject &reject_;
  QuadReject retail_;
  std::vector<Quad> pending_;
  uint32_t poly_ = 0u;
};

} // namespace

void RaceFlagDraw::draw(Core &core) const {
  const EmitMemory mem(core);
  const uint32_t gp = core.r[28];
  if (mem.mem_r16(gp + native::kRaceFlagCanDrawGpOffset) == 0u) {
    return;
  }
  const auto loadStage = [&] {
    return static_cast<int32_t>(mem.mem_r32(gp + native::kLoadRequestGpOffset));
  };
  if (static_cast<int32_t>(mem.mem_r32(gp + native::kRaceFlagTextFrameGpOffset)) < 0) {
    const int32_t stage = loadStage();
    if (stage > 5 && stage < 8) {
      mem.mem_w32(gp + native::kRaceFlagTextFrameGpOffset, 0u);
    }
  }
  if (static_cast<int32_t>(mem.mem_r32(gp + native::kRaceFlagTextFrameGpOffset)) >= 0) {
    guest::callToReturn(core, native::kRaceFlagDrawLoadingString, "CTR race flag loading string");
  }
  mem.mem_w32(gp + native::kRaceFlagLoadStageCopyGpOffset, static_cast<uint32_t>(loadStage()));
  guest::callToReturn(core, native::kRaceFlagOrderingTable, "CTR race flag ordering table");
  const uint32_t ot = core.r[2];

  for (uint32_t word = 0; word < gte::kRotationWords + gte::kTranslationWords; ++word) {
    core.gte().writeControl(gte::kRotation + word, mem.mem_r32(native::kRaceFlagMatrix + word * 4u));
  }
  core.gte().writeControl(gte::kOfx, kOffsetX);
  core.gte().writeControl(gte::kOfy, kOffsetY);
  core.gte().writeControl(gte::kH, kScreenDistance);

  const uint32_t tracker = mem.mem_r32(native::kGameTrackerPointer);
  const auto elapsedMs = static_cast<int32_t>(mem.mem_r32(tracker + native::kTrackerElapsedMsOffset));
  const uint32_t elapsed = mem.mem_r32(gp + native::kRaceFlagElapsedGpOffset);
  uint32_t columnPhase = static_cast<uint32_t>(static_cast<int32_t>(elapsed) >> 5);
  uint32_t ripple = columnPhase;

  Wave wave = loadWave(mem);
  uint32_t angle = stepWave(mem, wave, wave.speed * elapsedMs);
  storeWave(mem, wave);
  int32_t height = sine(mem, angle) + 0xFFF;
  int32_t leftLight = sine(mem, angle + kLightPhase) + 0xFFF;

  Column column;
  const auto firstX =
      static_cast<int16_t>(kFirstColumnX - static_cast<int16_t>(mem.mem_r16(gp + native::kRaceFlagPositionGpOffset)));
  column.x.fill(firstX);
  column.y = kTopY;
  column.depth = ((height * wave.amplitude) >> 13) + kDepth;
  for (int32_t row = 0; row < kRows; ++row) {
    projectRow(core, mem, column, ripple);
    lowerRow(column);
    storeRow(core, mem, native::kRaceFlagScreenBank + static_cast<uint32_t>(row) * kPointsPerRow * 4u);
  }

  const QuadReject reject(static_cast<uint32_t>(core.game->guestDisplay.plan().presentationHorizontalMargin));
  QuadEmitter quads(core, mem, {.tracker = tracker, .ot = ot, .root = orderingTableHead(core, tracker)}, reject);

  // Columns before retail's first continue its sequence backwards: x, ripple and wave phase are linear in the column.
  ColumnVertices inner{};
  for (std::size_t point = 0; point < inner.size(); ++point) {
    inner[point] = mem.loadVertex(native::kRaceFlagScreenBank + static_cast<uint32_t>(point) * 4u);
  }
  int32_t innerLight = leftLight;
  for (int32_t before = 1; before <= kMaxMarginColumns && reject.marginShows(wordsOf(inner)); ++before) {
    const auto beforeAngle = static_cast<uint32_t>(
        static_cast<int32_t>(static_cast<uint32_t>(wave.phase) - static_cast<uint32_t>(before * 64 * wave.speed)) >> 5);
    const int32_t light = sine(mem, beforeAngle + kLightPhase) + 0xFFF;
    Column outerColumn;
    outerColumn.x.fill(static_cast<int16_t>(firstX - before * kColumnStep));
    outerColumn.y = kTopY;
    outerColumn.depth = (((sine(mem, beforeAngle) + 0xFFF) * wave.amplitude) >> 13) + kDepth;
    uint32_t outerRipple = columnPhase - static_cast<uint32_t>(before) * kColumnPhaseStep;
    ColumnVertices outer{};
    for (int32_t row = 0; row < kRows; ++row) {
      projectRow(core, mem, outerColumn, outerRipple);
      lowerRow(outerColumn);
      for (uint32_t point = 0; point < kPointsPerRow; ++point) {
        outer[static_cast<std::size_t>(row) * kPointsPerRow + point] = core.gte().projected(point);
      }
    }
    for (int32_t segment = 0; segment + 1 < static_cast<int32_t>(kColumnPoints); ++segment) {
      const auto at = static_cast<std::size_t>(segment);
      if (!quads.emit({.column = 1 - before, .segment = segment},
                      {.previous = light, .current = innerLight},
                      {outer[at], outer[at + 1], inner[at], inner[at + 1]})) {
        quads.finish();
        return;
      }
    }
    inner = outer;
    innerLight = light;
  }

  uint32_t toggle = 1u;
  int32_t rightLight = leftLight;
  std::array<uint32_t, kColumnPoints> drawn{};
  for (int32_t columnIndex = 1; columnIndex < kColumns + kMaxMarginColumns; ++columnIndex) {
    const uint32_t bank = native::kRaceFlagScreenBank + toggle * kBankBytes;
    if (columnIndex >= kColumns) {
      mem.readWords(native::kRaceFlagScreenBank + (toggle ^ 1u) * kBankBytes, drawn);
      if (!reject.marginShows(drawn)) {
        break;
      }
    }
    uint32_t write = bank - 4u;
    toggle ^= 1u;
    uint32_t read = native::kRaceFlagScreenBank + toggle * kBankBytes;
    columnPhase += kColumnPhaseStep;
    ripple = columnPhase;
    angle = stepWave(mem, wave, wave.speed * 64);
    height = sine(mem, angle) + 0xFFF;
    leftLight = sine(mem, angle + kLightPhase) + 0xFFF;
    column.y = kTopY;
    for (int16_t &x : column.x) {
      x = static_cast<int16_t>(x + kColumnStep);
    }
    column.depth = ((height * wave.amplitude) >> 13) + kDepth;
    projectRow(core, mem, column, ripple);

    int32_t segment = 0;
    for (int32_t row = 1; row <= kRows; ++row) {
      storeRow(core, mem, write + 4u);
      if (row < kRows) {
        lowerRow(column);
        projectRow(core, mem, column, ripple);
      }
      int32_t quad = 0;
      if (segment == 0) {
        quad = 1;
        write += 4u;
      }
      for (; quad < kPointsPerRow; ++quad, ++segment) {
        const std::array<psx::gte::ProjectedVertex, 4> corners{
            mem.loadVertex(read), mem.loadVertex(read + 4u), mem.loadVertex(write), mem.loadVertex(write + 4u)};
        read += 4u;
        write += 4u;
        if (!quads.emit(
                {.column = columnIndex, .segment = segment}, {.previous = rightLight, .current = leftLight}, corners)) {
          quads.finish();
          return;
        }
      }
    }
    rightLight = leftLight;
  }
  quads.finish();
  mem.mem_w32(gp + native::kRaceFlagElapsedGpOffset, elapsed + static_cast<uint32_t>(elapsedMs * 100));
}

} // namespace ctr
