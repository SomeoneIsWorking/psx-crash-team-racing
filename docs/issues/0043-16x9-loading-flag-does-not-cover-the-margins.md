---
id: 43
title: At 16:9 the loading flag does not cover the margins
status: open
state_items: S006, S011
tags: widescreen,loading
created: 2026-10-29
updated: 2026-11-15
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
