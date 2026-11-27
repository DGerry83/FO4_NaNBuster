# Changelog

## 0.3.0.0

- Added one dual-runtime DLL for Fallout 4 OG 1.10.163 and AE 1.11.240, with exact-fingerprint pixel and compute hooks for each runtime.
- Added 13 exact AE shader identities and 27 offline-generated payloads through 27 appended registry entries, bringing the registry to 55 entries while preserving every OG identity and payload.
- Verified all 27 originals under all 16 INI policy combinations, deterministic payload generation, exact ASM/structural deltas, D3D11 creation, and all four OG/dual Debug/RelWithDebInfo build configurations with 26/26 tests in each.
- Verified on Nvidia AE: both hooks, all 13 substitutions, independent toggle behavior, ordinary rendering, and save/load with clean logs. AMD AE user confirmation remains pending; any dual release remains prerelease until that evidence exists and its removal is explicitly approved.

## 0.2.5.0

- Added exact-target isotropic specular anti-aliasing for original `f4b2da50-c279fcd9-bd60ba4f-6a804632`, widening the isotropic exponent from fine normal variance to reduce one-pixel glossy-edge strips and downstream bloom.
- Added independent default-on `IsotropicSpecularAA` configuration and one embedded registry entry, bringing the registry to 28 entries while preserving existing fixes and payloads.
- Verified the 7072-byte payload with exact DXBC/ASM tests, D3D11 creation, and RenderDoc replay: the confirmed peak fell from 35.7996788 to 14.8921652 and all sampled outputs remained finite.

## 0.2.4.0

- Expanded the anisotropic upper clamp proactively to all seven exact originals: 5612, 5620, 5617, 5650, 5832, 5836, and compute 6108.
- Preserved valid `[0,1]` lobe values and independent Lambert/anisotropic toggles; the registry now contains 27 entries.
- Verified 5620 replay with the offending base and power reduced to 1 while neighboring fragments remain unchanged.

## 0.2.3.0

- Added exact original compute shader 6108 to the anisotropic clamp and Lambert guard coverage, fixing a separate compute-lighting source of extreme finite HDR values and white-square bloom.
- Preserved independent Lambert and anisotropic toggles with three static payloads for 6108; the registry now contains 17 entries and twelve Lambert decoders.
- Verified compute replay with both offending lobe bases and powers reduced to 1 while valid comparison threads remain unchanged.

## 0.2.2.0

- Added an exact-target upper clamp for the 5612 anisotropic specular lobe, preventing out-of-range values from exploding into extreme finite HDR energy and white-square bloom.
- Added independent default-on `AnisotropicSpecularClamp` configuration. Static Lambert-only, clamp-only, and combined payloads preserve independent toggle behavior without runtime patch chaining.
- Verified corrected DXBC with focused tests, D3D11 creation, and RenderDoc replacement in the white-square capture.

## 0.2.1.0

- Added the uncovered 5836 Lambert decoder variant to the offline guard pipeline and embedded registry.
- Added independent OG compute-shader creation and replay hooks for 6404 with a verified 41-byte fingerprint and `0x90` vtable slot.
- Substitution logs now report the actual shader identity instead of always naming PS49B6A21CI0O1.

## 0.2.0.0

- Added ten offline-validated Lambert decoder guards that project invalid encoded normals into the valid decode disc, preventing a separate NaN-pixel cluster while leaving valid inputs unchanged.
- Added `NaNBuster.ini` with independent default-on `Color0LogClamp` and `LambertDecodeGuard` settings. The ten Lambert guards share one toggle, allowing either scoped fix to be disabled independently after restart.
- Added an immutable eleven-entry shader payload registry and policy-based exact-match selection while preserving the original COLOR0 payload and native creation hook.
- Added install-if-absent configuration staging so repeated development installs preserve user-edited INI files.

## 0.1.0.0

- Fixed OG plugin load failure ("Failed to get the plugin version data singleton"): the OG-only DLL now exports `F4SEPlugin_Version` (name/author/version sourced from CMake metadata) that the pinned CommonLibF4 AV resolves internally during `F4SE::Init`; the build-time export gate now requires that symbol alongside Query/Load.
- Added the OG-focused CommonLibF4 AV/CMake plugin skeleton.
- Added the verified original target DXBC and disassembly as read-only development references.
- Added `docs/Build-and-Compile.md` and `docs/Release.md`: development stages only into the ignored `dist/NaNBuster-dev/`, releases stage into `dist/NaNBuster/`, and local archives live in the ignored root `Releases/`; publication is gated on local ZIP testing and explicit user approval.
- Retired `package/` as an active staging destination (legacy contents preserved untouched).
- Added the offline four-channel DXBC clamp pipeline, deterministic hash regeneration, exact target matching, and embedded corrected payload.
- Added the OG shader-creation hook, original-creation replay, safe fallback behavior, and lifecycle diagnostics.
- Fixed the OG startup crash by reading the D3D output slot only after the displaced original instructions initialize it.
- Verified OG plugin loading, exact-target substitution, save/load, ordinary rendering, and controlled visual A/B removal of the black-square artifact.
