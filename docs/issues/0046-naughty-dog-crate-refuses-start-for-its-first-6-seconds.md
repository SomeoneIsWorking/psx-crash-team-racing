---
id: 46
title: The Naughty Dog crate refuses Start for its first 5.8 seconds
status: fixed
state_items: S011
tags: skip,boot,logo
created: 2026-11-17
updated: 2026-11-17
---

## Observed
Cold boot with Start pulsed every 32 fields (`PSXPORT_FORCE_BUTTONS=0008`, 16:9, ires 0): the crate logo runs from
about field 420 to 720 whatever is pressed, then Start ends it and the main menu's flag follows
(`scratch/ws-pass/start1/sheet.png`).

## Cause
The skip is retail's own: overlay 233's `CS_Thread_UseOpcode` (`0x800AC840`, read in ctr-native
`game/233/CS_Thread.c`, GPL, evidence only) takes `buttonsTapped & START`, and for `NAUGHTY_DOG_CRATE` refuses it
while `gGT->msInThisLEV >> 5 < 0xB5` (5,792 ms). Past that it stops the sequence music, calls
`CDSYS_XAPauseRequest`, puts the flag off screen and requests the main-menu level, which also skips the Oxide intro.

## Fix
`CrateLogoSkip` (`game/boot/crate_logo_skip.*`) owns overlay 233's `CS_Thread_UseOpcode` while that image is
resident. After the scene's own call (no instance), a Start or Cross tap on the crate with no load pending takes
retail's crate branch (decomp `docs/notes/cutscene-skip.md`); the call's camera step has run, so the frame under
the flag is the scene's own. `msInThisLEV` is untouched.

## Shown
Cold boot, Start pulsed (`PSXPORT_FORCE_BUTTONS=0008`), 16:9, ires 0: the crate scene's first frame is field 92 and
the flag covers it by field 130; the main menu shows at field 200, against 615 with only the voice-clip skip
(`scratch/ws-pass/crate4/sheet.png`, `scratch/ws-pass/crate2/sheet.png`).
