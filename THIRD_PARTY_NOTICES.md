# Third-Party Notices

NaNBuster is built with the help of the following open-source
projects and community work. This file distinguishes software linked into the
plugin, separately installed runtime prerequisites, and resources used only for
investigation and development. It also reproduces the license notices required
by the statically linked libraries.

NaNBuster's original source code is licensed under the MIT License — see the
`LICENSE` file in this repository. Third-party software, reference material,
and game shader assets retain their own copyrights and license terms.

---

## External runtime prerequisites — not redistributed

### Fallout 4 Script Extender (F4SE)

[F4SE](https://f4se.silverlock.org/) by Ian Patterson, Stephen Abel, and Brendan
Borthwick (ianpatt, behippo, and plb) loads NaNBuster and provides its plugin
runtime. The matching F4SE release must be installed separately. NaNBuster does
not redistribute the F4SE loader or runtime binaries.

The [F4SE source repository](https://github.com/ianpatt/f4se) and its
[readme and standard disclaimer](https://github.com/ianpatt/f4se/blob/master/f4se_readme.txt)
are provided by the F4SE authors. F4SE's readme asks that plugin source code be
publicly available; NaNBuster's repository provides its original source under
the MIT License.

### Address Library for F4SE Plugins

[Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/47327),
created by Ryan and maintained by its contributors, supplies version-independent
IDs and relocation data used by CommonLibF4 AV to resolve engine addresses at
runtime. It must be installed separately. No Address Library bin files or other
Address Library assets are included in NaNBuster's repository or release archive.

## Investigation and development resources — not runtime dependencies

### ShaderEngine, GFXBooster, and F4GFXBoosterShaderDB

[ShaderEngine](https://www.nexusmods.com/fallout4/mods/102022),
[GFXBooster](https://github.com/disi/GFXBooster), and the
[F4GFXBoosterShaderDB](https://github.com/disi/F4GFXBoosterShaderDB), by Disi
and contributors, provided shader metadata used during investigation. The
GFXBooster-produced database, linked from ShaderEngine, helped identify shader
UIDs, hashes, sizes, resource layouts, and package/assembly-hash correlations,
and helped prioritize sibling candidates.

The database supplied metadata, not the DXBC/assembly reference files used for
patching. No ShaderEngine or GFXBooster code is included, and neither tool is
required to run NaNBuster.

### RenderDoc

[RenderDoc](https://renderdoc.org/) captures, resource inspection, pixel history,
and shader debugging supplied the original DXBC used for offline patching and
established both NaN mechanisms. It was also used to compare original and
corrected rendering behavior. RenderDoc is a development and diagnostic tool;
it is not a runtime dependency and no RenderDoc code is bundled.

### Addictol

[Addictol](https://github.com/Dear-Modding-FO4/Addictol), by the Dear-Modding-FO4
contributors, provided an initial native shader-creation reverse-engineering
lead. NaNBuster's hook sites and behavior were independently verified. No
Addictol code is included, and Addictol is not required.

### northaxosky's CommonLibF4 template

The [northaxosky CommonLibF4 template](https://github.com/northaxosky/commonlibf4-template)
informed F4SE entry-point and plugin-structure decisions.

### Community Shaders

[Community Shaders](https://github.com/northaxosky/fallout4-community-shaders),
by northaxosky and its contributors, was consulted to compare its Effect shader
source and determine whether it addressed or shared the same vertex-color power
issue. It was investigative reference material, not the basis of the main
RenderDoc/DXBC investigation. **No Community Shaders code is used in NaNBuster.**

Community Shaders can replace the same Effect shader family, so it remains a
likely candidate for overlapping behavior or problem reports. Interoperation is
untested and testing is pending; neither compatibility nor incompatibility is
claimed. NaNBuster does not force precedence over third-party replacement
shaders.

## Fallout 4 shader assets

Fallout 4 and its original shader assets are the property of Bethesda
Softworks/ZeniMax and their respective owners. The development repository
contains read-only original DXBC/disassembly references extracted from the
user's game captures for interoperability research and validation. Corrected
shader payloads derived from those originals are embedded in NaNBuster.dll;
standalone reference DXBC files are not included in the release archive. These
assets are not covered by NaNBuster's MIT License, and no ownership of them is
claimed.

## Statically linked libraries and reproduced license notices

### CommonLibF4 (All Versions fork)

The plugin is statically linked against the "All Versions" fork of
CommonLibF4 maintained by LucaDotGit
(<https://github.com/LucaDotGit/CommonLibF4>), building on CommonLibF4 work by
ryan-rsm-mckenzie and its contributors. CommonLibF4 AV provides the plugin
lifecycle, relocation, hooking, and logging framework used by NaNBuster and is
licensed under the MIT License:

> MIT License
>
> Copyright (c) 2019 ryan-rsm-mckenzie
>
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.

### {fmt}

<https://github.com/fmtlib/fmt> — statically linked via vcpkg. Licensed under
the MIT License:

> Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors
>
> Permission is hereby granted, free of charge, to any person obtaining
> a copy of this software and associated documentation files (the
> "Software"), to deal in the Software without restriction, including
> without limitation the rights to use, copy, modify, merge, publish,
> distribute, sublicense, and/or sell copies of the Software, and to
> permit persons to whom the Software is furnished to do so, subject to
> the following conditions:
>
> The above copyright notice and this permission notice shall be
> included in all copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
> EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
> MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
> NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
> LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
> OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
> WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
>
> --- Optional exception to the license ---
>
> As an exception, if, as a result of your compiling your source code, portions
> of this Software are embedded into a machine-executable object form of such
> source code, you may redistribute such embedded portions in such object form
> without including the above copyright and permission notices.

### spdlog

<https://github.com/gabime/spdlog> — statically linked via vcpkg. Licensed
under the MIT License:

> The MIT License (MIT)
>
> Copyright (c) 2016 Gabi Melman.
>
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.
>
> -- NOTE: Third party dependency used by this software --
> This software depends on the fmt lib (MIT License),
> and users must comply to its license: https://raw.githubusercontent.com/fmtlib/fmt/master/LICENSE
