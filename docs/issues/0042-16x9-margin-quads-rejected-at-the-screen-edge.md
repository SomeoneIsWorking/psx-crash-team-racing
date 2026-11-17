---
id: 42
title: At 16:9 the outer margin loses whole quads, leaving a sawtooth edge
status: fixed
state_items: S006
tags: widescreen,race,culling
created: 2026-10-29
updated: 2026-11-17
---

## Observed
`warp 3` (Crash Cove grid) at 16:9, ires 0, sink 1920x1080, 800 fields after the warp: the rock arch on the right
and the wall and road on the left end in a sawtooth about 30-50 canvas columns short of the 684 edge, with sky or
black behind. 4:3 at the same field is complete. Evidence: `scratch/readme-media/p1_800.png`.

## Cause
Overlay 226's level draw (`DrawLevelOvr1P`) loads its GTE state from the view at `0x800A0D34`: OFX = `rect.w`/2
and H from the view, and the packed clip window from `rect` +4 (w, h) into scratchpad. Each quad and triangle is then
rejected when all its packed screen words have the X sign bit set (wholly left of 0) or all `sxy - window` are
non-negative (wholly at or right of `w`). The left test is a sign bit, so no view data can move it; only quads that
straddle 0 or 511 survive, which is the sawtooth. The 2P-4P overlays (227-229) share the same setup. Evidence:
ctr-native (GPL, read only) `game/226/226_00_DrawLevelOvr1P.c` `IsProjectedPolyOffscreenPacked` and
`Ovr226_800a0d34_SetEntryGteAndCameraScratch`.

## Proper fix
A native owner for the level draw's clip that tests the plan's drawn columns (`guestClipLeft`..`guestClipRight`)
in place of the packed window, drawn in the sandboxed wide pass issue 0045 needs, so the guest's own run stays at
retail width and cost.

## Known
- Not only the frustum: building `0x800430F0`'s full-width view at 684 brought back the banner's right end but not
  these quads (that widening is withdrawn, issue 0045).
- Not `H`: `H` stays retail, so a quad in the margin has guest screen X below 0 or above 511.
- Issue 0032 measured 8.7% fewer `0x30`/`0x34` prims at 16:9 under the old zoomed plan.

## Fixed 2026-11-17 (1P)
`LevelDrawMargins` (`game/video/level_draw_margins.*`) leaves the guest's call retail's (width, time, pool, table
links), then runs the same call again inside a `GuestRedraw` (guest time held, every store put back) from the same
table and pool with the view rect spanning the drawn columns. The polygons retail's window rejects are read out of
that run and drawn as host packets, moved back by the left margin with their vertex places, in their buckets after
the call's own. Drawing wide in the guest's own call cost guest time and pool: the race moved (clock +1, AI drivers
apart at field 6,000), and the larger frame starved the loading flag's pool on a mid-race load.
The 2P-4P draws (overlays 227-229) are not owned yet.
