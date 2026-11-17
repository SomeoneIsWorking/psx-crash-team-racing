---
id: 43
title: At 16:9 the loading flag does not cover the margins
status: fixed
state_items: S006, S011
tags: widescreen,loading
created: 2026-10-29
updated: 2026-11-17
---

## Observed
`warp 3` at 16:9, 120 fields after the warp: the checkered loading flag covers the centre 512 columns. Its right
edge is a sawtooth, and the previous track shows through both margins. At 4:3 the flag fills the screen. Evidence:
`scratch/readme-media/l1_120.png`.

## Cause
`RaceFlag_DrawSelf` (`0x800444E8`) sets its own projection (OFX 256, OFY 120, H 256), transforms a 35-column mesh
and keeps a quad only when not all four corners are left of 0 or above 0 (sign mask `0x80008000`, `0x800448B4`) and
not all are at or past (512, 216) (`0x00D80200` in `$s0`, `0x800448CC`/`0x800448E8`). Whole quads that land in a
margin are dropped.

The mesh is a 3D grid of 35 columns, 100 units apart, from x = -0xBBE minus the slide offset `gp+0x4D8`, with
three rows per strip at y -910, -816 and -722 plus 0x11A per strip, at z 0x280 plus the wave
(`0x800444E8` reading, `env-assets/decomp/ctr/c/800444E8.c`). The slide offset moves the whole grid across
the screen, so the margins are covered whenever the projected grid reaches past them; the reach at full cover
is not measured. Its one caller is `0x80037830` in `FUN_80035E70`.

## Proper fix
A native body for `0x800444E8` that keeps a quad unless it lies wholly outside the plan's drawn columns
(`guestClipLeft`..`guestClipRight`) or rows 0..215, with the guest's packets, OT link and pool cursor otherwise as
retail writes them.

## Fix
`game/video/race_flag_draw.cpp:RaceFlagDraw::draw` owns `0x800444E8` as a native body: retail's wave, grid, colours,
packets, pool cursor and OT link, with the reject applied to X measured from the plan's left margin over a window
`512 + 2 * margin` wide (identical to retail at 4:3). Where the grid's end column lies wholly beyond one side of
retail's 512 columns and still reaches into that margin, the grid continues by whole columns: past column 34 with
retail's own wave step, before column 0 by the same linear sequence run backwards (x -100, ripple -0x100, phase
-64 * speed per column). The same body draws the main menu's flag, which had the same sawtooth.

## Shown
Cold boot, 16:9, ires 0, 4K sink, unpaced: `scratch/ws-pass/flagL/before_after.png` (left HEAD `1aa7b7e`, right the
fix) at fields 900 (LOADING), 1000, 1100 and 1200 (main menu), and `flagL/diff1200.png`: the new pixels are all in
the margins, and the left margin is flag in every frame with no sawtoothed edge. At 1000 and 1100 the menu flag's own
waving end crosses guest column 512, so no column is added and the margin past it stays black; drawing there would
change the 4:3 picture. At 1200 the end lies past 512 and the right margin fills.
