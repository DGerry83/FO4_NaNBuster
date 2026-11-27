# NaNBuster

NaNBuster is a standalone Fallout 4 F4SE plugin that prevents two independent sources of NaN pixels, an anisotropic-specular range failure that can produce white-square bloom artifacts, and valid-normal isotropic specular aliasing that can produce needle-like glossy-edge highlights. It substitutes corrected shader bytecode only for exact original size/hash matches and leaves all other shaders unchanged.

**Current prerelease:** [v0.3.0](https://github.com/DGerry83/FO4_NaNBuster/releases/tag/v0.3.0), supporting Fallout 4 OG 1.10.163 and AE 1.11.240. All four scoped fixes have independent, default-on INI toggles.

**Verification:** AE loading, both hooks, all 13 shader substitutions, independent toggles, ordinary rendering, and save/load are verified on Nvidia. AMD AE user confirmation remains pending, so this dual release remains prerelease until that evidence exists and removal of prerelease status is explicitly approved.

## Requirements

- Windows x64 and Steam Fallout 4 **OG 1.10.163** or **AE 1.11.240**, supported by the v0.3.0 dual prerelease.
- [Fallout 4 Script Extender (F4SE)](https://f4se.silverlock.org/) matching the game.
- [Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/47327), installed separately. NaNBuster does not package its bin files.

## Installation

Install the release ZIP with MO2 or another mod manager that supports the standard `F4SE/Plugins/` layout, then enable the mod.

For a manual install, extract the archive so `NaNBuster.dll` and `NaNBuster.ini` are placed in Fallout 4's `Data/F4SE/Plugins/` directory. Keep your existing INI when upgrading if you have customized it. Restart Fallout 4 after installing, upgrading, or changing settings.

## Scoped fixes

### COLOR0 logarithm clamp

- Shader: `PS49B6A21CI0O1`
- RenderDoc/DXBC hash: `82f7e159-8db6b7a4-b742fff3-b11324e9`
- Original DXBC size: 1612 bytes

The source-level correction for the corresponding shader is:

```hlsl
float4 col = pow(max(IN.param2, 0.0), 2.2);
```

All four interpolated COLOR0 channels are clamped before the existing logarithm/multiply/exponential sequence. Other instructions and the resource interface are preserved.

### Lambert decode guard

Twelve shaders—ten pixel shaders and two compute shaders—decode normals with `e = sample * 4 - 2`, followed by `f = dot(e,e)` and `sqrt(1 - f/4)`. Values outside the valid disc can otherwise produce NaN. The guard selects `1.999 * rsqrt(f)` only when `f > 4`, scales `e.xy`, and recomputes `f` before the original decode. Valid inputs are unchanged; invalid inputs produce a unit normal within floating-point rounding.

The plugin embeds all corrected DXBC payloads in the DLL. No runtime HLSL compilation or binary rewriting occurs. Malformed data, unexpected versions, and third-party replacement shaders pass through unchanged. Additional sibling shaders and fog-specific patches are not included.

### Anisotropic specular clamp

- Pixel shader resources: `5612`, `5620`, `5617`, `5650`, `5832`, and `5836`
- Compute shader resource: `6108`, original hash `be21279d-733505d8-43f98a86-09502d65`, 6588 bytes

The second anisotropic specular lobe is clamped to `[0,1]` before its exponent-125 evaluation. This prevents a non-unit sampled direction from producing extreme finite HDR energy that subsequent downsample and bloom passes spread into white squares. Values already within the valid range are unchanged. Static Lambert-only, clamp-only, and combined payloads preserve independent behavior for the Lambert and anisotropic toggles.

### Isotropic specular anti-aliasing

- Pixel shader hash: `f4b2da50-c279fcd9-bd60ba4f-6a804632`
- Original DXBC size: 6528 bytes
- Corrected DXBC size: 7072 bytes

Fine screen-space normal derivatives estimate local normal variance and widen the isotropic Blinn-Phong exponent where glossy edges change rapidly. The anisotropic branch, existing 15.0 clamp, early-out, resource interface, and nine temporary registers remain unchanged. This addresses valid-normal specular aliasing rather than NaN or out-of-range input; it does not add Lambert decoder coverage.

RenderDoc replay of the exact payload reduced the confirmed shader peak from 35.7996788 to 14.8921652 while allowing a dark shoulder to rise, reducing the one-pixel strip and its downstream bloom rather than globally dimming the image.

## Configuration

`NaNBuster.ini` is installed beside `NaNBuster.dll` under `F4SE/Plugins/` only when no file already exists. Existing user settings are preserved on repeated development installs.

```ini
[Fixes]
Color0LogClamp=true
LambertDecodeGuard=true
AnisotropicSpecularClamp=true
IsotropicSpecularAA=true
```

Each fix defaults to enabled. Set only the problematic fix to `false` to disable it while leaving the other fixes active. `LambertDecodeGuard` controls all twelve Lambert shaders as one fix; `AnisotropicSpecularClamp` controls the seven exact anisotropic originals: 5612, 5620, 5617, 5650, 5832, 5836, and 6108. `IsotropicSpecularAA` controls the exact `f4b2da50...` isotropic anti-aliasing payload independently. Missing settings and invalid values default to enabled; invalid values also produce a startup warning in the NaNBuster log. Changes require restarting Fallout 4. Substitution logs report the actual shader identity.

## Community Shaders interoperability

Community Shaders was consulted during investigation to compare its Effect shader source and determine whether it addressed or shared the same vertex-color power issue. **No Community Shaders code is used in NaNBuster**, and Community Shaders was not the basis of the main RenderDoc/DXBC investigation.

Community Shaders remains relevant because it can replace the same Effect shader family and is therefore a likely candidate for overlapping behavior or problem reports. NaNBuster does not patch unfamiliar replacement bytecode or force precedence over it.

**Interoperation with Community Shaders:** basic OG 1.10.163 coexistence was tested on 2026-11-26 with the dual-runtime development candidate. The user observed normal shader behavior and no visual issues, and the NaNBuster log confirmed all four fixes, both hooks, expected substitutions, and no errors. Community Shaders version coverage is not exhaustive, and no separate Community Shaders log is required. NaNBuster does not patch unfamiliar replacement bytecode or force precedence over it.

## Investigation and development resources

NaNBuster's implementation and validation used the following community projects and tools:

- **[ShaderEngine](https://www.nexusmods.com/fallout4/mods/102022), [GFXBooster](https://github.com/disi/GFXBooster), and the [F4GFXBoosterShaderDB](https://github.com/disi/F4GFXBoosterShaderDB), by Disi and contributors.** The GFXBooster-produced shader database, linked from ShaderEngine, supplied shader UIDs, hashes, sizes, resource layouts, and package/assembly-hash correlations. These metadata helped identify the original target and prioritize sibling candidates. The database did not supply the DXBC/assembly reference files used for patching. **ShaderEngine and GFXBooster are not required by the shipped DLL, and no code from either project is included.**
- **[RenderDoc](https://renderdoc.org/).** Captures, resource inspection, pixel history, and shader debugging supplied the original DXBC used for offline patching and established both NaN mechanisms. RenderDoc was also used to compare original and corrected rendering behavior. It is a development and diagnostic tool, not a runtime dependency.
- **[Addictol](https://github.com/Dear-Modding-FO4/Addictol).** Its native shader-creation work provided an initial reverse-engineering lead. NaNBuster's hook sites and behavior were independently verified; Addictol is not required.
- **[northaxosky's CommonLibF4 template](https://github.com/northaxosky/commonlibf4-template).** The template informed F4SE entry-point and plugin-structure decisions.
- **F4SE, CommonLibF4 AV, Address Library, {fmt}, and spdlog.** F4SE loads the plugin; CommonLibF4 AV provides the statically linked plugin/relocation framework; Address Library supplies the separately installed runtime relocation data; {fmt} and spdlog support logging.

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for the corresponding credits, distribution distinctions, and reproduced license notices.

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

Ordinary development staging goes to the ignored `dist/NaNBuster-dev/` folder (the user-owned MO2 symlink target):

```bash
"C:/Users/Matt/source/repos/TOOLS/cmake-4.4.3/bin/cmake.exe" --install build/og-msvc --config RelWithDebInfo --prefix dist/NaNBuster-dev
```

See `docs/Build-and-Compile.md` for the full build, test, export, and staging guide, and `docs/Release.md` for release staging (`dist/NaNBuster/`), local archives (`Releases/`), and the approval-gated publication procedure.

## Layout

- `src/Core/` — pure DXBC and target-matching logic.
- `src/Application/` — shader-selection use case.
- `src/Infrastructure/` — F4SE, hooking, D3D, payload, and logging adapters.
- `tools/ShaderPatcher/` — offline COLOR0 DXBC generator.
- `tools/LambertShaderPatcher/` — offline Lambert guard generator.
- `tools/IsotropicShaderPatcher/` — offline isotropic specular-AA generator.
- `tools/ShaderTriage/` — offline static shader candidate analysis.
- `tests/` — layer-mirrored tests.
- `references/shaders/PS49B6A21CI0O1/` — read-only original COLOR0 DXBC and disassembly.
- `references/shaders/LambertDecode/` — read-only original Lambert DXBC/disassembly pairs.
- `references/shaders/IsotropicSpecularAA/` — read-only original isotropic DXBC/disassembly.
- `references/shaders/AEShaderVariants/` — read-only original AE variant DXBC inputs.
- `build/shaders/AEShaderVariants/` — generated corrected AE DXBC, disassembly, and embedded include data.
- `assets/config/NaNBuster.ini` — default-on scoped-fix configuration.
- `dist/NaNBuster-dev/` — ignored development staging; the user-owned MO2 symlink target.
- `dist/NaNBuster/` — ignored release staging; recreated only for explicit releases.
- `Releases/` — ignored local ZIP archives and release notes.
- `package/` — legacy staging payload (retired; preserved untouched, never refreshed).
- `build/` — generated intermediates and corrected shader output.
- `.flybywire/` — ignored workflow artifacts.

See `AGENTS.md` for agent onboarding and verification constraints.

## License and acknowledgements

NaNBuster's original source code is licensed under the MIT License; see [LICENSE](LICENSE). Third-party software, reference material, and game shader assets remain the property of their respective authors and owners. Credits and third-party license notices are collected in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
