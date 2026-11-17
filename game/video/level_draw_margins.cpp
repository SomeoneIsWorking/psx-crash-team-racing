#include "video/level_draw_margins.h"

#include "core.h"
#include "emit_memory.h"
#include "execution/guest_turns.h"
#include "frame/ordering_table_head.h"
#include "game.h"
#include "gp0_primitive_decode.h"
#include "guest_redraw.h"
#include "r3000.h"
#include "record_geometry.h"
#include "title/native_ownership.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <lucent/log.h>
#include <span>
#include <vector>

namespace ctr {
namespace {

using psx::present::EmitMemory;

constexpr uint32_t kLinkMask = 0x00FFFFFFu;
constexpr uint32_t kRamBase = 0x80000000u;

// A margin polygon of the wide run: drawn just before `anchor`, the first node older than the call in its bucket.
struct MarginPacket {
  uint32_t anchor = 0;
  std::array<uint32_t, 0xFF> words{};
  std::array<psx::gte::WordPlace, 0xFF> places{};
  uint32_t count = 0;
  uint32_t site = 0;
};

uint32_t linkOf(uint32_t tag) {
  return (tag & kLinkMask) | kRamBase;
}

// A wide-run packet retail's window would reject: wholly left of column 0 or at or right of `width`, once moved
// back by `margin`.
bool marginOnly(std::span<const uint32_t> words, int margin, int width) {
  const auto primitive = psx::gpu::decodePacketPrimitive({.words = words, .places = {}});
  if (!primitive) {
    return false;
  }
  const psx::gpu::RecordRect bounds = psx::gpu::primitiveBounds(*primitive);
  return bounds.x1 - margin <= 0 || bounds.x0 - margin >= width;
}

// Each bucket's chain from its entry runs through the wide run's packets [first, end) to the first older node; the
// margin polygons among them, moved back by `margin`, in walk order.
std::vector<MarginPacket>
collectMargins(const EmitMemory &mem, uint32_t ot, uint32_t first, uint32_t end, int margin, int width) {
  std::vector<MarginPacket> margins;
  for (uint32_t entry = 0; entry < native::kOrderingTableEntries; ++entry) {
    const std::size_t bucketStart = margins.size();
    uint32_t node = linkOf(mem.mem_r32(ot + entry * 4u));
    for (uint32_t guard = 0; node >= first && node < end; ++guard) {
      if (guard > (end - first) / 4u) {
        lucent::error("ctr-level", "DrawLevelOvr1P's wide chain from entry {} does not leave its packets", entry);
        std::abort();
      }
      const uint32_t tag = mem.mem_r32(node);
      MarginPacket packet;
      packet.count = tag >> 24;
      const auto words = std::span(packet.words.data(), packet.count);
      mem.readWords(node + 4u, words);
      if (marginOnly(words, margin, width)) {
        const std::array<uint32_t, 0xFF> drawn = packet.words;
        if (!psx::gpu::moveVerticesX(words, -margin)) {
          lucent::error("ctr-level", "DrawLevelOvr1P packet 0x{:08X} holds a command whose vertices cannot move", node);
          std::abort();
        }
        const auto places = std::span(packet.places.data(), packet.count);
        mem.readPlaces(node + 4u, places);
        for (uint32_t word = 0; word < packet.count; ++word) {
          const psx::gte::WordPlace place = places[word];
          if (packet.words[word] != drawn[word] && place.has_value()) {
            places[word] = psx::gte::ProjectedVertex(drawn[word], place.value()).steppedTo(packet.words[word]).place();
          }
        }
        packet.site = mem.placingSite(node + 4u, std::span(drawn.data(), packet.count));
        margins.push_back(packet);
      }
      node = linkOf(tag);
    }
    for (std::size_t i = bucketStart; i < margins.size(); ++i) {
      margins[i].anchor = node;
    }
  }
  return margins;
}

} // namespace

void LevelDrawMargins::draw(Core &core) const {
  const EmitMemory mem(core);
  const std::array<uint32_t, 4> arguments{
      core.r[psx::cpu::kA0], core.r[psx::cpu::kA1], core.r[psx::cpu::kA2], core.r[psx::cpu::kA3]};
  const uint32_t view = arguments[native::kViewArgumentRegister - psx::cpu::kA0];
  const uint32_t primMem = arguments[native::kPrimMemArgumentRegister - psx::cpu::kA0];
  const auto &plan = core.game->guestDisplay.plan();
  const int nativeWidth = plan.nativeExtent.width;
  const uint16_t width = mem.mem_r16(view + native::kViewRectWidthOffset);
  const bool fullWidth = mem.mem_r16(view + native::kViewRectXOffset) == 0u && width == nativeWidth;
  const int margin = plan.presentationHorizontalMargin;
  if (!fullWidth || margin <= 0) {
    guest::callOriginalToReturn(core, native::kLevelDraw1P, "DrawLevelOvr1P");
    return;
  }
  const uint32_t ot = mem.mem_r32(view + native::kViewOrderingTableOffset);
  std::array<uint32_t, native::kOrderingTableEntries> heads{};
  mem.readWords(ot, heads);
  const uint32_t first = mem.mem_r32(primMem + native::kPrimMemCursorOffset);
  guest::callOriginalToReturn(core, native::kLevelDraw1P, "DrawLevelOvr1P");

  // The same call again from the same table and pool with guest time held and the view's rect spanning the drawn
  // columns (OFX = w / 2 moves right by the margin, the packed window reaches the right margin's edge). Every store
  // goes back; the polygons only it drew are read out first.
  std::vector<MarginPacket> margins;
  {
    psx::present::GuestRedraw redraw(core);
    for (uint32_t entry = 0; entry < heads.size(); ++entry) {
      if (mem.mem_r32(ot + entry * 4u) != heads[entry]) {
        redraw.writeWord(ot + entry * 4u, heads[entry]);
      }
    }
    redraw.writeWord(primMem + native::kPrimMemCursorOffset, first);
    redraw.writeHalf(view + native::kViewRectWidthOffset, static_cast<uint16_t>(nativeWidth + 2 * margin));
    if (!redraw.run(native::kLevelDraw1P, arguments)) {
      return;
    }
    margins = collectMargins(mem, ot, first, mem.mem_r32(primMem + native::kPrimMemCursorOffset), margin, nativeWidth);
  }
  const uint32_t root = orderingTableHead(core, mem.mem_r32(native::kGameTrackerPointer));
  for (const MarginPacket &packet : margins) {
    core.hostPackets.insertBefore(core,
                                  root,
                                  packet.anchor,
                                  {.words = std::span(packet.words.data(), packet.count),
                                   .places = std::span(packet.places.data(), packet.count)},
                                  packet.site);
  }
}

} // namespace ctr
