# 0040 — A disc read holds one present for 200 ms

Status: fixed (2026-10-28). Owner: `game/disc/async_disc_owner.cpp` (`DiscReadOwner::deliverIfDue`).

## Observed
Every all-title 40 s paced hidden run reported one present interval of 180-200 ms in the attract loop. Unpaced
(`scratch/player-bugs-3/r1`, `PSXPORT_DEBUG=presentseq`, 1600 fences) the same field, fence 1435 to 1436, took
202 ms of host time, so it is the field's cost, not pacing.

## Cause
The field issues a 243-sector `BIGFILE.BIG` read (LBA 52753) through `FUN_800321b4` with completion callback
`0x80032110`, then polls for that callback inside the same field. `deliverIfDue` held the callback until the
drive's read time had passed in emulated CPU time: 243 sectors at double speed, about 1.6 s of guest time. The guest
spun through that whole wait in one field (17 polls 308k ticks apart, `scratch/player-bugs-3/r3`), costing 200 ms of
host time. Disc reads are instant by design; the drive time was only there so the loader publishes its state
before the callback runs.

## Fix
The completion is owed until the loader that called `CdRead` has returned: `noteTransferComplete` records the
guest stack pointer at the `CdRead` call, and the next budget exit with the stack above it delivers the callback.
The drive-time wait is gone.

## Shown
Same run after the fix (`scratch/player-bugs-3/r4`, 3000 fences unpaced): no field longer than 30 ms after boot
(the remaining 59 ms at fence 446 is the renderer start, not a read); callback 30 is still delivered in the same
field 1435, so the attract loop's timing is unchanged, and the next present shows the LOAD screen
(`scratch/player-bugs-3/evidence/0040_f1437.png`).
