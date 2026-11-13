# Prior art

Outside work on CTR USA `SCUS_944.26`; a reference names addresses, our measurement decides behaviour.

Our target: `SCUS_944.26`, SHA-256 `7b4aac0bf2f6310984e599295df17b457da5a23b270c20200cefef6079efb838`, load `0x80010000`.
Address checks ran `decomp_pipeline.py --function-at` on our image (2026-10-13): `0x8007781C` (3 insns), `0x80073398`
(28), `0x80075350` (94), `0x80024C4C` (body to `0x80025137`, matching our lens-flare producer boundary) are all
function entries. Licences, trees and push dates were re-read through the GitHub API on 2026-11-13.

## Decomp (submodule)

| submodule | URL | pinned | licence | target build and match | progress (verified in tree) | what we may take |
|---|---|---|---|---|---|---|
| `external/ctr-native` | https://github.com/CTR-tools/ctr-native | `8e21ed5d` (2026-10-06; repo last push 2026-11-01) | GPL-3.0 | build `926` = USA retail: `metadata/retail/ntsc-u-926/matching.json` lists `SCUS_944.26` with our SHA-256 and size 516096; overlay load addresses in `metadata/retail/ntsc-u-926/disc.json` (221/222 at `0x8009F6FC`, as in `titles/ctr/overlays.json`); symbols `metadata/retail/ntsc-u-926/symbols/syms926.txt` | non-matching decomp: 326 `game/**/*.c` covering the EXE and overlays 221-233, compiled natively; `match.sh` and `tools/matching/` drive a byte-matching track (gcc 2.8.1, maspsx) with no published percentage | evidence only (GPL): names, struct layouts, function semantics; no code in our tree |

It is the active home of the CTR-ModSDK decompilation (ModSDK's README moved decomp work here and to CTR-in-C).
Verified features: 16:9 behind `USE_16BY9` (`main.c`, `platform/native_renderer.c`); a temporary 60 fps path in
`game/MAIN/MainFrame_RenderFrame.c` that runs the logic at 60 Hz, a different design from our 30 Hz replay interpolation.

## Other projects

| project | URL | kind | licence | target build | build match (addresses checked) | what we may take | notes |
|---|---|---|---|---|---|---|---|
| CTR-tools/CTR-ModSDK | https://github.com/CTR-tools/CTR-ModSDK | modding SDK plus non-matching decomp (`decompile/`) | GPL-3.0 | build `926` = USA retail | matched: `symbols/gcc-syms926.txt` `SetGeomScreen 0x8007781C`, `SetGeomOffset 0x8007782C`, `VSync 0x80075350`, `DrawOTag 0x80073398`, `PushBuffer_SetPsyqGeom 0x80042910`, `FLARE_ThTick 0x80024C4C`, `RenderBucket_Execute 0x8006AAA8`, `start 0x8007793C` | evidence only (names) | Last push 2026-07-05. Verified 60 fps at 60 Hz logic in `decompile/General/AltMods/60fps/` (`USE_60FPS`, rate-dependent sites gated on `gGT->timer` bits) and 16:9 via `USE_16BY9`; its gated sites are a list of rate-dependent logic to check. |
| CTR-tools/CTR-in-C | https://github.com/CTR-tools/CTR-in-C | non-matching decomp, fork of CTR-ModSDK | none in tree; fork of a GPL-3.0 repo | USA `926` | not checked directly (fork of ModSDK) | evidence only (names) | `FUNCTIONS.MD`, `STRUCTURES.MD`. Last push 2026-08-04. |
| kkv0n/CTR-Matching | https://github.com/kkv0n/CTR-Matching | splat configs and function lists, EXE plus overlays 221-233 | none | NTSC-U `SCUS-94426` (no hash recorded) | matched: `syms/funcs_SCUS_94426.txt` has entries at `0x8007781C`, `0x8007782C`, `0x80075350`, `0x80073398`, `0x80042910`, `0x80024C4C`, `0x8006AAA8`, `0x8007793C`; overlay vram 225 `0x8009F6FC`, 226 `0x800A0CB8`, 230 `0x800AB9F0` match `titles/ctr/overlays.json` | evidence only | Proof of concept, last push 2026-08-12. |
| Decryptu/ctr-decrypted | https://github.com/Decryptu/ctr-decrypted | ctr-native derivative with enhancements | GPL-3.0 | USA `926` via ctr-native | not checked directly | evidence only | Verified widescreen in `game/widescreen.c`, `include/widescreen.h`; 60 fps (`docs/60FPS.md`) is 60 Hz logic ported from High Octane. Last push 2026-11-12. |
| Rinnegatamante/Crash-Team-Racing-High-Octane | https://github.com/Rinnegatamante/Crash-Team-Racing-High-Octane | ctr-native fork (Vita and PC) | GPL-3.0 | USA `926` via ctr-native | not checked directly | evidence only | Source of the 60 Hz-logic 60 fps mode other forks take. Last push 2026-10-31. |
| chrissotraidis/ctrpad | https://github.com/chrissotraidis/ctrpad | ctr-native port to macOS/iOS/iPadOS | GPL-3.0 | USA `926` via ctr-native | not checked directly | evidence only | Last push 2026-10-27. |
| NyperYuhgard/CTR-PC-Port | https://github.com/NyperYuhgard/CTR-PC-Port | native PC port from copies of the ModSDK source | no LICENSE file; carries GPL-3.0 ModSDK-derived code | USA via ModSDK | not checked | evidence only | Last push 2026-08-03. |
| LorenzoAlfaro/CTRStaticRecomp | https://github.com/LorenzoAlfaro/CTRStaticRecomp | static recompilation (MIPS to C, HLE runtime) | MIT | USA `SCUS-94426` plus overlays 221-233 | not checked directly: seeds from ModSDK `syms926.txt` | may copy with attribution; little fits a dynarec port | Last push 2026-10-26. |
| CTR-tools/CTR-tools | https://github.com/CTR-tools/CTR-tools | C# format parsers (BIGFILE, LEV, models) | GPL-2.0 | data formats, all regions | not applicable | evidence only | Last push 2026-10-04. |
| mateusfavarin/CrashTeamEditor | https://github.com/mateusfavarin/CrashTeamEditor | track editor | MIT | data formats | not applicable | may copy with attribution (level format) | Last push 2026-03-13. |
| icebound777/CTR-Randomizer-Standalone | https://github.com/icebound777/CTR-Randomizer-Standalone | randomizer | MIT | not stated | not checked | may copy with attribution | Last push 2026-06-28. |

Not found (2026-11-13): a byte-matching CTR decomp with a published percentage; a permissively licensed CTR decomp
or symbol map; a 30 Hz-logic interpolated 60 fps mode anywhere (every 60 fps mode found runs the logic at 60 Hz).
Searched GitHub repositories ("crash team racing", "ctr decomp", "ctr native", serial), decomp.dev, and the web.
A search result proxied through `*.web.val.run` with credentials in the URL (2026-10-13) was not opened.
