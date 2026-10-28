---
id: 44
title: CTR has no 60 fps interpolation
status: open
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
