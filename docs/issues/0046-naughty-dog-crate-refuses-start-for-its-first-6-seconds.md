---
id: 46
title: The Naughty Dog crate refuses Start for its first 5.8 seconds
status: open
state_items: S011
tags: skip,boot,logo
created: 2026-11-17
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

## Proper fix
A title-owned skip that takes that same branch (music stop, XA fade, flag, `MainRaceTrack_RequestLoad(main menu)`)
on a Start or Cross edge before the gate, through an override of the overlay function that owns the branch, keyed to
overlay 233's image. Writing `msInThisLEV` forward is not a fix.
