# Changelog

## Unreleased

- Fixed OG plugin load failure ("Failed to get the plugin version data singleton"): the OG-only DLL now exports `F4SEPlugin_Version` (name/author/version sourced from CMake metadata) that the pinned CommonLibF4 AV resolves internally during `F4SE::Init`; the build-time export gate now requires that symbol alongside Query/Load.
- Added the OG-focused CommonLibF4 AV/CMake plugin skeleton.
- Added the verified original target DXBC and disassembly as read-only development references.
- Added `docs/Build-and-Compile.md` and `docs/Release.md`: development stages only into the ignored `dist/NaNBuster-dev/`, releases stage into `dist/NaNBuster/`, and local archives live in the ignored root `Releases/`; publication is gated on local ZIP testing and explicit user approval.
- Retired `package/` as an active staging destination (legacy contents preserved untouched).
- Shader correction and engine hooking are pending implementation.
