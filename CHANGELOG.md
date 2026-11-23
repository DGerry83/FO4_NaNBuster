# Changelog

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
