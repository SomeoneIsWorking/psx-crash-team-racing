# 0041 — The frame loop is not owned

Status: open (2026-10-28). Owner: `game/frame/frame_driver.cpp:FrameDriver::stepFrame`.

## What the code does
Each step dispatches main-loop state 3 `0x8003C58C` (`kGuestMain`) for one field and resumes the owed continuation at the guest's next VSync or budget exit (`dispatchField`).

## Why it matters
The workspace rule (psxport `docs/workspace/WORKSPACE.md`, "Each title owns its frame loop") wants the frame driver to run the guest's per-frame update and draw and decide the cadence, replacing the guest's VSync waits. Until then the loop is on the fallback; it is not yet on `Timing::vsyncFields` either, since every guest VSync is answered with one field whatever its argument.

## What blocks it
The state-3 body is run as guest code up to its waits; the per-frame update and draw it calls are not split out of `0x8003C58C`. Next: decompile `0x8003C58C`'s state-3 arm and drive its update/draw pair natively.
