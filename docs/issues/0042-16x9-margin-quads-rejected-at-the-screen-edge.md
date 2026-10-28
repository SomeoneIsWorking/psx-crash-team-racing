---
id: 42
title: At 16:9 the outer margin loses whole quads, leaving a sawtooth edge
status: open
state_items: S006
tags: widescreen,race,culling
created: 2026-10-29
---

## Observed
`warp 3` (Crash Cove grid) at 16:9, ires 0, sink 1920x1080, 800 fields after the warp: the rock arch on the right
and the wall and road on the left end in a sawtooth about 30-50 canvas columns short of the 684 edge, with sky or
black behind. 4:3 at the same field is complete. Evidence: `scratch/readme-media/p1_800.png`.

## Known
- Not the frustum: `0x800430F0` now builds a full-width view at 684 (`ViewFrustumOwner`), which brought back the
  banner's right end but not these quads.
- Not `H`: `H` stays retail, so a quad in the margin has guest screen X below 0 or above 511.
- The drop is per quad, so a screen-space reject in the submitters is likely. Issue 0032 measured 8.7% fewer
  `0x30`/`0x34` prims at 16:9 under the old zoomed plan.

## Next
Find the reject in the ten submitters (`docs/re-frontier.md` CTR-05). The four `H` readers (`cfc2 $26` at
`0x8006A6B8`, `0x80070AF8`, `0x80070EE8`, `0x80071150`) derive bounds from `H`, so check them first. Widen the
bound in a title owner by the plan's margin, the same way `ViewFrustumOwner` widens the frustum.
