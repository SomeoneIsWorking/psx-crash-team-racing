---
id: 44
title: CTR has no 60 fps interpolation
status: fixed
state_items: S007
tags: fps60,interpolation
created: 2026-10-29
---

## Observed
With `fps60=1`, each game frame is still presented for two fields: the Record path declares no interpolation
(`composed=0`, S012), so the port runs at retail 30 fps. Without in-betweens, the README cannot show an
interpolation comparison.

## Proper fix
Previous/current pairs for the camera (the view `0x80042910` publishes) and for each driver and instance transform,
keyed by object identity. In-betweens are composed from those pairs at the halfway point, and a `SceneCut` holds
the pair across loads and warps. This follows the workspace's draw-site tagging rule: explicit ids, lerped
transforms, whole-frame coverage or none.

## Fix
Replay interpolation (psxport `docs/plans/replay-interpolation.md`): `DrawSection` re-runs RenderFrame's picture
half at each in-between with `DrawReplay`'s tracks blended (views, instances, drivers). Crash Cove replay, 8x, 4K
sink, unpaced: 2898 intervals replayed, 3 refused (device accesses), 2865 in-betweens shown; a `?` crate's x across
real/in-between/real reads 213/197/180 and 145/133/120 in crop pixels. Main-RAM digests over 2617 fields are
identical with fps60 on and off. Still held: the loading screen (its chain is not chained) and records whose chain
bakes an upload (0x800F5D28, about 30 per run).
