# 0040 — A disc read holds one present for 200 ms

Status: open (2026-10-27). Owner: not yet attributed (the disc read that completes libcd read callback 30).

## Observed
Every all-title 40 s paced hidden run reports one present interval of 180-200 ms behind a small missed share:
`worst=200.28ms` (`scratch/player-bugs/realtime40`), `200.26ms` (`scratch/player-bugs-2/realtime40`), `183.60ms`
(`scratch/player-bugs-2/realtime40-3`), each `beyond_range=1`.

## Classification
A paced run with `PSXPORT_DEBUG=presentseq` (`scratch/player-bugs-2/ctr-worst/run.log`, 40 s from boot): the one long
gap is fence 1435 to 1436, 200.3 ms, at about 24 s into the attract loop. Fence 1436 was shown 0.4 ms after its due
time, so the pacer was not late; the field itself took 200 ms of host time. The only line inside the gap is
`[ctr-disc] delivered libcd read-completion callback 30 -> 0x80032110`, 30 ms before the present. CD reads are meant
to be instant, so a 170 ms read or decode on the host is the first suspect.

## Next
Time the disc read behind callback 30 (sector count, CHD hunks decoded) against the field's host time.
