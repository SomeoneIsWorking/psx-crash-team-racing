# 0047 — Guest addresses from the decomp symbol map

RE knowledge lives in the decomp; the port reads guest addresses from its symbol map as generated constants, never a
hand copy. CTR's decomp is `external/ctr-decomp` (MIT, ours); its map is `config/symbols/<image>.txt`, splat
`symbol_addrs` format, one file per image (`scus_944.26`, `ov221`..`ov233`). It holds every address
`game/title/native_ownership.h` names, under the community's function names where they exist
(`docs/our-names.md` there lists the port's earlier names beside them).

Not started: the generator is shared with Tomba! 2's plan (psx-tomba 0065) and lands in psxport once, for both.

## Today

- 79 `inline constexpr` address literals in `game/title/native_ownership.h`, plus 8 file-local return sites in
  `game/boot/startup_resource_load.cpp` (3), `game/boot/startup_resource_pump.cpp` (4) and one bit mask in
  `game/video/race_flag_draw.cpp` (not an address; it stays).
- The frame driver's field-override table keys its 10 entries on those constants.
- Struct field and gp offsets (`k*Offset`) are documented in the decomp's `docs/structures.md`; the gp-relative ones
  are also absolute sdata symbols in the map (`g_MainState` = gp+0x188 and so on).

## Generator

psxport `tools/symbol_constants.py` with a CMake function `psxport_symbol_constants(<target> IMAGE <name>
MAP <file>...)`, as psx-tomba plan 0065 specifies: one header per image under `build/`,
`namespace ctr::sym::scus_944_26 { inline constexpr std::uint32_t <name> = 0x...u; }`, `size` where the map gives it.
Labels (`type:label`) are generated like functions, so return sites need no `<function> + 0xNN` arithmetic.

## Order

1. Generator in psxport (if Tomba! 2 has not landed it), then `ctr_port` builds the `scus_944.26` header that nothing
   includes yet.
2. `native_ownership.h`: each address constant becomes a use of the generated symbol, one owner group per commit
   (boot, frame, disc/CD, video/projection, race flag); the constant names adopt the map's where they differ.
3. The file-local return sites in `game/boot/` use the generated labels.
4. Overlay addresses (overlay 226 renderer, overlay 233 cutscene thread) are named in `ov<n>.txt` first and reach
   the port through `ctr::sym::ov<n>`, keyed to the image generation the override already checks.
5. Build with Clang and boot to the main menu once to show no binding moved.
