---
id: 45
title: At 16:9 the margins lose geometry the retail cull leaves out
status: open
state_items: S006
tags: widescreen,race,culling
created: 2026-10-29
---

## Observed
With the guest's own cull, geometry outside the 4:3 view frustum is never submitted, so the 16:9 margins show sky or
the clear colour where the scene continues (the banner's right end on the Crash Cove grid, `warp 3`).

## Withdrawn fix
`ViewFrustumOwner` ran the retail frustum builder `0x800430F0` with a full-width view's rect width raised to 684. It
filled the banner, but the race no longer matched 4:3: replaying `replays/gameplay/arcade-crash-cove-drive.pad`
(fps60=0, unpaced, paused and stepped to each field), the 16:9 race clock read 448 against 416 at field 5,400 and
10,048 against 10,016 at 6,000, and every driver position differed by 6,940. With the builder at retail width the
clocks and all eight driver positions are equal at 5,400, 6,000, 6,500 and 6,940
(`scratch/review-fixes/race_state.py`). The owner is removed.

## Proper fix
Draw margin-only geometry from a list beside retail's: run the cull at retail width for the guest, and submit what a
684-wide frustum adds as host-side primitives that write no guest memory and spend no guest time.
