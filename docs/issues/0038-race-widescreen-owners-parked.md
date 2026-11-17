---
id: 38
title: Race widescreen owners are parked, not landed
status: fixed
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

## Fixed 2026-11-17
`HudAnchor` (`game/video/hud_anchor.*`) owns `UI_RenderFrame_Racing`'s element drawers and the `DecalFont_DrawLine`
and `DecalHUD_DrawPolyGT4` calls it makes itself; each element's packets move to the edge they sit on through psxport's
`anchorHudElement`. The rank suffix and the wumpa count each lead a group: the HUD instance beside it (the rank digit
`instBigNum`, the fruit `instFruitDisp`), drawn later by `RenderBucket_Execute`, takes the same shift. Its packets are
found by running the render list again in a `GuestRedraw` from the call's start, stopped before and after that
instance, and checked word for word against the call's own before they move.
Shown on the Crash Cove replay at 16:9 (`scratch/ws-pass/hud5-169/full_5900.png`; the two groups over four consecutive
presents in `scratch/ws-pass/hud5-169/groups.png`). The
parked owner is not used.
