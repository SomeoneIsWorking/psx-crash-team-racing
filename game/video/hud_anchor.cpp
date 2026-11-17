#include "video/hud_anchor.h"

#include "core.h"
#include "execution/guest_turns.h"
#include "frame/frame_driver.h"
#include "frame/ordering_table_head.h"
#include "game.h"
#include "gte_control.h"
#include "guest_redraw.h"
#include "hud_anchor_packets.h"
#include "r3000.h"
#include "title/native_ownership.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <lucent/log.h>
#include <optional>
#include <utility>
#include <vector>

namespace ctr {
namespace {

// One element: a drawer, or a shared drawer only when UI_RenderFrame_Racing calls it directly; a group's lead sets the
// shift its instance takes.
struct Element {
  uint32_t address = 0;
  const char *name = nullptr;
  bool fromRaceFrameOnly = false;
  std::optional<HudAnchor::Group> group;
};

constexpr std::array kElements{
    Element{.address = native::kUiMapDrawMap, .name = "HUD UI_Map_DrawMap"},
    Element{.address = native::kUiMapDrawDrivers, .name = "HUD UI_Map_DrawDrivers"},
    Element{.address = native::kUiMapDrawGhosts, .name = "HUD UI_Map_DrawGhosts"},
    Element{.address = native::kUiMapDrawTracking, .name = "HUD UI_Map_DrawTracking"},
    Element{.address = native::kUiDrawRaceClock, .name = "HUD UI_DrawRaceClock"},
    Element{.address = native::kUiDrawPosSuffix, .name = "HUD UI_DrawPosSuffix", .group = HudAnchor::Group::Rank},
    Element{.address = native::kUiDrawLapCount, .name = "HUD UI_DrawLapCount"},
    Element{.address = native::kUiWeaponDrawSelf, .name = "HUD UI_Weapon_DrawSelf"},
    Element{.address = native::kUiWeaponDrawBg, .name = "HUD UI_Weapon_DrawBG"},
    Element{.address = native::kUiDrawNumWumpa, .name = "HUD UI_DrawNumWumpa", .group = HudAnchor::Group::Wumpa},
    Element{.address = native::kUiDrawNumTimebox, .name = "HUD UI_DrawNumTimebox"},
    Element{.address = native::kUiDrawSpeedNeedle, .name = "HUD UI_DrawSpeedNeedle"},
    Element{.address = native::kUiDrawSpeedBg, .name = "HUD UI_DrawSpeedBG"},
    Element{.address = native::kUiJumpMeterDraw, .name = "HUD UI_JumpMeter_Draw"},
    Element{.address = native::kUiDrawSlideMeter, .name = "HUD UI_DrawSlideMeter"},
    Element{.address = native::kUiDrawRankedDrivers, .name = "HUD UI_DrawRankedDrivers"},
    Element{.address = native::kDecalFontDrawLine, .name = "HUD DecalFont_DrawLine", .fromRaceFrameOnly = true},
    Element{.address = native::kDecalHudDrawPolyGt4, .name = "HUD DecalHUD_DrawPolyGT4", .fromRaceFrameOnly = true},
};

constexpr std::array<uint32_t, static_cast<std::size_t>(HudAnchor::Group::Count)> kGroupInstanceOffsets{
    native::kDriverRankDigitOffset, native::kDriverWumpaFruitOffset};

// Longer than any list the instance pool can queue.
constexpr uint32_t kRenderListLimit = 0x10000u;
constexpr uint32_t kPacketCountShift = 24u;
constexpr uint32_t kPacketLinkMask = 0x00FFFFFFu;

HudAnchor &owner(Core &core) {
  return core.game->frameDriverAs<FrameDriver>().hudAnchor();
}

bool calledFromRaceFrame(const Core &core) {
  const uint32_t caller = core.r[psx::cpu::kRa];
  return caller >= native::kUiRenderFrameRacing && caller < native::kUiRenderFrameRacingEnd;
}

template <std::size_t Index> void drawAnchored(Core *core) {
  constexpr Element element = kElements[Index];
  Core &c = *core;
  if (element.fromRaceFrameOnly && !calledFromRaceFrame(c)) {
    guest::callOriginalToReturn(c, element.address, element.name);
    return;
  }
  const uint32_t tracker = c.mem_r32(native::kGameTrackerPointer);
  const uint32_t buffer = c.mem_r32(tracker + native::kTrackerBackBufferOffset);
  const uint32_t first = c.mem_r32(buffer + native::kBufferPrimCursorOffset);
  guest::callOriginalToReturn(c, element.address, element.name);
  const int dx =
      psx::present::anchorHudElement(c,
                                     orderingTableHead(c, tracker),
                                     {.first = first, .end = c.mem_r32(buffer + native::kBufferPrimCursorOffset)},
                                     c.game->guestDisplay.plan().nativeExtent.width / 2);
  if constexpr (element.group.has_value()) {
    owner(c).shiftGroup(*element.group, dx);
  }
}

void drawInstances(Core *core) {
  owner(*core).drawInstances(*core);
}

template <std::size_t... Index> constexpr auto makeBindings(std::index_sequence<Index...>) {
  return std::array<FieldOverrideBinding, sizeof...(Index) + 1>{
      {{.address = kElements[Index].address, .name = kElements[Index].name, .function = drawAnchored<Index>}...,
       {.address = native::kRenderBucketExecute, .name = "HUD RenderBucket_Execute", .function = drawInstances}}};
}

constexpr auto kBindings = makeBindings(std::make_index_sequence<kElements.size()>{});

// A shifted group's instance at its place in the render list.
struct Target {
  uint32_t entry = 0;
  int dx = 0;
};

// Its packets in the pool, [first, end), and their words as the sandboxed run drew them.
struct Drawn {
  uint32_t first = 0;
  uint32_t end = 0;
  int dx = 0;
  std::vector<uint32_t> words;
};

} // namespace

std::span<const FieldOverrideBinding> HudAnchor::bindings() {
  return kBindings;
}

void HudAnchor::shiftGroup(Group group, int dx) {
  shifts_[static_cast<std::size_t>(group)] = dx;
}

void HudAnchor::drawInstances(Core &core) {
  const uint32_t list = core.r[psx::cpu::kA0];
  const uint32_t primMem = core.r[psx::cpu::kA1];
  const psx::present::GteControl gte = psx::present::readGteControl(core.gte());
  const uint32_t first = core.mem_r32(primMem + native::kPrimMemCursorOffset);
  guest::callOriginalToReturn(core, native::kRenderBucketExecute, "RenderBucket_Execute");
  const auto shifts = std::exchange(shifts_, {});
  if (std::ranges::all_of(shifts, [](int dx) {
        return dx == 0;
      })) {
    return;
  }
  const uint32_t tracker = core.mem_r32(native::kGameTrackerPointer);
  const uint32_t driver = core.mem_r32(tracker + native::kTrackerDriversOffset);
  std::vector<Target> targets;
  for (uint32_t entry = 0; core.mem_r32(list + entry * native::kRenderListEntryBytes) != 0u; ++entry) {
    if (entry == kRenderListLimit) {
      lucent::error("ctr-hud", "RenderBucket_Execute list 0x{:08X} has no end", list);
      std::abort();
    }
    const uint32_t instance =
        core.mem_r32(list + entry * native::kRenderListEntryBytes + native::kRenderListInstanceOffset);
    for (std::size_t group = 0; group < shifts.size(); ++group) {
      if (shifts[group] != 0 && driver != 0u && instance == core.mem_r32(driver + kGroupInstanceOffsets[group])) {
        targets.push_back({.entry = entry, .dx = shifts[group]});
      }
    }
  }
  if (targets.empty()) {
    return;
  }

  // The list again from the call's start with the GTE as the call found it, ended before and after each target, so
  // the cursor between the stops bounds that instance's packets.
  std::vector<Drawn> drawn;
  {
    psx::present::GuestRedraw redraw(core);
    psx::present::writeGteControl(core.gte(), gte);
    redraw.writeWord(primMem + native::kPrimMemCursorOffset, first);
    const auto runTo = [&](uint32_t from, uint32_t to) {
      const uint32_t stop = list + to * native::kRenderListEntryBytes;
      const uint32_t header = core.mem_r32(stop);
      redraw.writeWord(stop, 0u);
      const std::array<uint32_t, 2> arguments{list + from * native::kRenderListEntryBytes, primMem};
      if (!redraw.run(native::kRenderBucketExecute, arguments)) {
        lucent::error("ctr-hud", "RenderBucket_Execute entries {}..{} did not draw in a redraw", from, to);
        std::abort();
      }
      redraw.writeWord(stop, header);
      return core.mem_r32(primMem + native::kPrimMemCursorOffset);
    };
    uint32_t from = 0;
    for (const Target &target : targets) {
      Drawn packets{.first = runTo(from, target.entry), .dx = target.dx};
      packets.end = runTo(target.entry, target.entry + 1u);
      for (uint32_t address = packets.first; address < packets.end; address += 4u) {
        packets.words.push_back(core.mem_r32(address));
      }
      drawn.push_back(std::move(packets));
      from = target.entry + 1u;
    }
  }

  const uint32_t root = orderingTableHead(core, tracker);
  for (const Drawn &packets : drawn) {
    for (uint32_t packet = packets.first; packet < packets.end;) {
      const uint32_t tag = core.mem_r32(packet);
      const uint32_t words = (tag >> kPacketCountShift) + 1u;
      for (uint32_t word = 0; word < words; ++word) {
        const uint32_t mask = word == 0u ? ~kPacketLinkMask : ~0u;
        const uint32_t retail = core.mem_r32(packet + word * 4u);
        if (((retail ^ packets.words[(packet - packets.first) / 4u + word]) & mask) != 0u) {
          lucent::error("ctr-hud", "HUD instance packet 0x{:08X} differs from its redraw at word {}", packet, word);
          std::abort();
        }
      }
      packet += words * 4u;
    }
    psx::present::moveHudElement(core, root, {.first = packets.first, .end = packets.end}, packets.dx);
  }
}

} // namespace ctr
