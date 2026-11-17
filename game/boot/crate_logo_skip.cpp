#include "boot/crate_logo_skip.h"

#include "core.h"
#include "execution/guest_turns.h"
#include "r3000.h"
#include "title/native_ownership.h"

#include <array>
#include <cstdint>

namespace ctr {
namespace {

uint32_t call(Core &core, uint32_t address) {
  guest::callToReturn(core, address, "CTR crate skip");
  return core.r[psx::cpu::kV0];
}

bool skipTapped(Core &core, uint32_t tracker) {
  if (core.mem_r32(tracker + native::kTrackerLevelIdOffset) != native::kNaughtyDogCrateLevel) {
    return false;
  }
  const uint32_t pads = core.mem_r32(native::kGamepadsPointer);
  return (core.mem_r32(pads + native::kGamepadTappedOffset) & (native::kButtonStart | native::kButtonCross)) != 0u;
}

} // namespace

void CrateLogoSkip::useOpcode(Core *core) {
  Core &c = *core;
  const uint32_t scene = c.r[psx::cpu::kA0];
  const uint32_t cutscene = c.r[psx::cpu::kA1];
  std::array<uint32_t, native::kCutsceneOpcodeWords> opcode{};
  for (uint32_t i = 0; i < opcode.size(); ++i) {
    opcode[i] = c.mem_r32(cutscene + native::kCutsceneOpcodeOffset + 4u * i);
  }
  guest::callOriginalToReturn(c, native::kCutsceneUseOpcode, "CS_Thread_UseOpcode");
  const uint32_t tracker = c.mem_r32(native::kGameTrackerPointer);
  if (scene != 0u || !skipTapped(c, tracker) ||
      c.mem_r32(c.r[psx::cpu::kGp] + native::kLoadRequestGpOffset) != native::kNoLoadRequest) {
    return;
  }
  const uint32_t clock = tracker + native::kTrackerClockEffectOffset;
  c.mem_w16(clock, static_cast<uint16_t>(c.mem_r16(clock) & ~native::kClockEffectCameraPath));
  c.r[psx::cpu::kA0] = 1u;
  call(c, native::kRaceFlagSetCanDraw);
  if (call(c, native::kRaceFlagIsTransitioning) == 0u && call(c, native::kRaceFlagIsFullyOnScreen) == 0u) {
    call(c, native::kRaceFlagSetFullyOffScreen);
  }
  call(c, native::kCseqMusicStopAll);
  call(c, native::kXaPauseRequest);
  c.r[psx::cpu::kA0] = 0u;
  call(c, native::kRaceFlagSetDrawOrder);
  c.r[psx::cpu::kA0] = native::kMainMenuLevel;
  call(c, native::kRequestLevelLoad);
  c.mem_w32(native::kCutsceneFinished, 1u);
  const uint32_t mode2 = tracker + native::kTrackerGameMode2Offset;
  c.mem_w32(mode2, c.mem_r32(mode2) & ~native::kGameMode2FreezePodium);
  for (uint32_t i = 0; i < opcode.size(); ++i) {
    c.mem_w32(cutscene + native::kCutsceneOpcodeOffset + 4u * i, opcode[i]);
  }
  c.r[psx::cpu::kV0] = 1u;
}

} // namespace ctr
