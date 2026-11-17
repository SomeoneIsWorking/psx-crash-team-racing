#include "video/hud_anchor.h"

#include "core.h"
#include "execution/guest_turns.h"
#include "frame/ordering_table_head.h"
#include "game.h"
#include "hud_anchor_packets.h"
#include "r3000.h"
#include "title/native_ownership.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace ctr {
namespace {

// One element: a drawer, or a shared drawer only when UI_RenderFrame_Racing calls it directly.
struct Element {
  uint32_t address = 0;
  const char *name = nullptr;
  bool fromRaceFrameOnly = false;
};

// The rank suffix and the wumpa count stay: each sits beside a 3D instance (the big rank digit, the fruit) the
// instance renderer draws in place.
constexpr std::array kElements{
    Element{.address = native::kUiMapDrawMap, .name = "HUD UI_Map_DrawMap"},
    Element{.address = native::kUiMapDrawDrivers, .name = "HUD UI_Map_DrawDrivers"},
    Element{.address = native::kUiMapDrawGhosts, .name = "HUD UI_Map_DrawGhosts"},
    Element{.address = native::kUiMapDrawTracking, .name = "HUD UI_Map_DrawTracking"},
    Element{.address = native::kUiDrawRaceClock, .name = "HUD UI_DrawRaceClock"},
    Element{.address = native::kUiDrawLapCount, .name = "HUD UI_DrawLapCount"},
    Element{.address = native::kUiWeaponDrawSelf, .name = "HUD UI_Weapon_DrawSelf"},
    Element{.address = native::kUiWeaponDrawBg, .name = "HUD UI_Weapon_DrawBG"},
    Element{.address = native::kUiDrawNumTimebox, .name = "HUD UI_DrawNumTimebox"},
    Element{.address = native::kUiDrawSpeedNeedle, .name = "HUD UI_DrawSpeedNeedle"},
    Element{.address = native::kUiDrawSpeedBg, .name = "HUD UI_DrawSpeedBG"},
    Element{.address = native::kUiJumpMeterDraw, .name = "HUD UI_JumpMeter_Draw"},
    Element{.address = native::kUiDrawSlideMeter, .name = "HUD UI_DrawSlideMeter"},
    Element{.address = native::kUiDrawRankedDrivers, .name = "HUD UI_DrawRankedDrivers"},
    Element{.address = native::kDecalFontDrawLine, .name = "HUD DecalFont_DrawLine", .fromRaceFrameOnly = true},
    Element{.address = native::kDecalHudDrawPolyGt4, .name = "HUD DecalHUD_DrawPolyGT4", .fromRaceFrameOnly = true},
};

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
  psx::present::anchorHudElement(c,
                                 orderingTableHead(c, tracker),
                                 {.first = first, .end = c.mem_r32(buffer + native::kBufferPrimCursorOffset)},
                                 c.game->guestDisplay.plan().nativeExtent.width / 2);
}

template <std::size_t... Index> constexpr auto makeBindings(std::index_sequence<Index...>) {
  return std::array<FieldOverrideBinding, sizeof...(Index)>{
      {{.address = kElements[Index].address, .name = kElements[Index].name, .function = drawAnchored<Index>}...}};
}

constexpr auto kBindings = makeBindings(std::make_index_sequence<kElements.size()>{});

} // namespace

std::span<const FieldOverrideBinding> HudAnchor::bindings() {
  return kBindings;
}

} // namespace ctr
