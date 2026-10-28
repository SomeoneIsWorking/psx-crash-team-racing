---
id: 38
title: Race widescreen owners are parked, not landed
status: open
tags: widescreen,race,hud
created: 2026-10-14
---

## State
An unfinished race widescreen pass (view frustum owner, HUD anchor owner, projection distance scale,
frame driver hooks) was parked uncommitted so the boot restructure could land. It failed
`ctr_widescreen_owner` ("H is scaled… got 8192, expected 10944") because `projection_owner.cpp` set the
distance scale to 1/1.

## Where it is
`scratch/parked-race-widescreen/` (gitignored, kept by `.scratch-keep`): `tracked.diff` against
`cf6c4a2` plus the four new `game/video/*_owner.*` files.

## Landed 2026-10-29
The distance scale is 1/1: scaling `H` zoomed the picture. The frustum owner is `game/video/view_frustum_owner.*`.
The rest is tracked in issue 0042 (margin quads) and issue 0043 (loading flag).

## Next
The HUD anchor owner (`scratch/parked-race-widescreen/hud_anchor_owner.*`): re-apply it on the current frame
driver, so edge-anchored HUD items move into the margins.
