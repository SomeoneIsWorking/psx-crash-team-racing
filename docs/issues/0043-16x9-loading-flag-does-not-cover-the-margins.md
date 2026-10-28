---
id: 43
title: At 16:9 the loading flag does not cover the margins
status: open
state_items: S006, S011
tags: widescreen,loading
created: 2026-10-29
---

## Observed
`warp 3` at 16:9, 120 fields after the warp: the checkered loading flag covers the centre 512 columns. Its right
edge is a sawtooth, and the previous track shows through both margins. At 4:3 the flag fills the screen. Evidence:
`scratch/readme-media/l1_120.png`.

## Cause (not yet traced)
The flag's mesh is authored to span the 4:3 view, and with `H` retail nothing scales it into the margins.

## Next
Find the flag's producer and the view it draws through, and widen its horizontal extent by the plan's ratio.
