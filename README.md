# NaNBuster

NaNBuster is a standalone Fallout 4 F4SE plugin that prevents two independent NaN sources: the original COLOR0 logarithm issue and invalid Lambert normal decoding.

**Current status:** development version 0.2.1.0. The original OG correction and creation hook remain intact, and this version adds eleven Lambert decoder guards with independent INI toggles.

## Scoped fixes

### COLOR0 logarithm clamp

- Shader: `PS49B6A21CI0O1`
- RenderDoc/DXBC hash: `82f7e159-8db6b7a4-b742fff3-b11324e9`
- Original DXBC size: 1612 bytes
- Initial runtime: Fallout 4 OG 1.10.163

The source-level correction for the corresponding shader is:

```hlsl
float4 col = pow(max(IN.param2, 0.0), 2.2);
```

### Lambert decode guard

Eleven shaders decode normals with `e = sample * 4 - 2`, followed by `f = dot(e,e)` and `sqrt(1 - f/4)`. Values above the valid disc can otherwise produce NaN. The guard selects `1.999 * rsqrt(f)` only when `f > 4`, scales `e.xy`, and recomputes `f` before the original decode. Valid inputs are unchanged; invalid inputs produce a unit normal within floating-point rounding.

The standalone plugin embeds all corrected DXBC payloads in the DLL. It does not require Shader Engine, GFXBooster, or Community Shaders. Only exact original size/hash matches are substituted; all other shaders pass through unchanged. AE and sibling shader patches are not included.

## Configuration

`NaNBuster.ini` is installed beside `NaNBuster.dll` under `F4SE/Plugins/` only when no file already exists. Existing user settings are preserved on repeated development installs.

```ini
[Fixes]
Color0LogClamp=true
LambertDecodeGuard=true
```

Each fix defaults to enabled. Set only the problematic fix to `false` to disable it while leaving the other fix active. `LambertDecodeGuard` controls all eleven Lambert shaders as one fix. Missing settings and invalid values default to enabled; invalid values also produce a startup warning. Changes require restarting Fallout 4. Substitution logs report the actual shader identity.

The likely Community Shaders source location is [`package/Shaders/Effect.hlsl`](https://github.com/northaxosky/fallout4-community-shaders/blob/7df9173df2cf97089e009ff7cbef52ac1118f502/package/Shaders/Effect.hlsl#L305); the compiled-variant mapping is not yet verified.

## Requirements

- Windows x64 and Steam Fallout 4 1.10.163 for the MVP.
- The F4SE release matching the game.
- [Address Library for F4SE](https://www.nexusmods.com/fallout4/mods/47327), downloaded and installed separately. NaNBuster does not package its bin files.

## Build

Prerequisites:

- CMake 4.3 or newer (the verified portable copy is 4.4.3; see the commands below).
- Visual Studio 2026 with Desktop development with C++.
- vcpkg with `VCPKG_ROOT` set.
- Git for the CommonLibF4 AV submodule.

From the repository root in Git Bash:

```bash
"C:/Users/Matt/source/repos/TOOLS/cmake-4.4.3/bin/cmake.exe" --preset og-msvc
"C:/Users/Matt/source/repos/TOOLS/cmake-4.4.3/bin/cmake.exe" --build --preset og-msvc-release
"C:/Users/Matt/source/repos/TOOLS/cmake-4.4.3/bin/ctest.exe" --preset og-msvc-release
```

Ordinary development staging goes to the ignored `dist/NaNBuster-dev/` folder (the
user-owned MO2 symlink target):

```bash
"C:/Users/Matt/source/repos/TOOLS/cmake-4.4.3/bin/cmake.exe" --install build/og-msvc --config RelWithDebInfo --prefix dist/NaNBuster-dev
```

See `docs/Build-and-Compile.md` for the full build, test, export, and staging guide, and
`docs/Release.md` for release staging (`dist/NaNBuster/`), local archives (`Releases/`),
and the approval-gated publication procedure.

## Layout

- `src/Core/` — pure DXBC and target-matching logic.
- `src/Application/` — shader-selection use case.
- `src/Infrastructure/` — F4SE, hooking, D3D, payload, and logging adapters.
- `tools/ShaderPatcher/` — offline COLOR0 DXBC generator.
- `tools/LambertShaderPatcher/` — offline Lambert guard generator.
- `tests/` — layer-mirrored tests.
- `references/shaders/PS49B6A21CI0O1/` — read-only original COLOR0 DXBC and disassembly.
- `references/shaders/LambertDecode/` — read-only original Lambert DXBC/disassembly pairs.
- `assets/config/NaNBuster.ini` — default-on scoped-fix configuration.
- `dist/NaNBuster-dev/` — ignored development staging; the user-owned MO2 symlink target.
- `dist/NaNBuster/` — ignored release staging; recreated only for explicit releases.
- `Releases/` — ignored local ZIP archives and release notes.
- `package/` — legacy staging payload (retired; preserved untouched, never refreshed).
- `build/` — generated intermediates and corrected shader output.
- `.flybywire/` — ignored workflow artifacts; the approved design is in the finished bootstrap session and the implementation plan is in `active/2026-11-18_NaNBusterMVP_G1-G8/`.

See `AGENTS.md` for agent onboarding and verification constraints.

## License

MIT; see `LICENSE`.
