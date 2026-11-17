# Crash Team Racing measured target

The selected target is the North American disc whose `SYSTEM.CNF` names
`cdrom:\SCUS_944.26;1`.

| Field | Measured value |
|---|---|
| Executable | `SCUS_944.26` |
| Disc extent | LBA 24, 516,096 bytes |
| SHA-256 | `7b4aac0bf2f6310984e599295df17b457da5a23b270c20200cefef6079efb838` |
| Entry | `0x8007793C` |
| Load address | `0x80010000` |
| Text extent | `[0x80010000,0x8008D800)` |
| Header stack | `0x801FFFF0` |

`executable.json` holds these identity facts for the build and `tools/provision.py`.

The image layout (sections, crt0, overlays), the symbol map and the RE evidence chain are in the decomp,
`../../external/ctr-decomp/docs/images.md` and `../../external/ctr-decomp/docs/re-frontier.md`.

The product maps this authenticated executable into psxport's Lightrec executor. Runtime activation
of the measured `BIGFILE.BIG` images remains incomplete. Do not emit, build, or run generated guest
code, and do not expose an interpreter mode. See `../../docs/migration.md`.
