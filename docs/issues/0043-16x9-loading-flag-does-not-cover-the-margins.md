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
margin are dropped; whether the mesh reaches the 684 edges is not measured.

## Proper fix
A native body for `0x800444E8` that keeps a quad unless it lies wholly outside the plan's drawn columns
(`guestClipLeft`..`guestClipRight`) or rows 0..215, with the guest's packets, OT link and pool cursor otherwise as
retail writes them.
