cmake_minimum_required(VERSION 4.3.0)

set(COMMONLIB_PLUGIN_NAME "NaNBuster")
set(COMMONLIB_PLUGIN_AUTHOR "DGerry83")
set(COMMONLIB_PLUGIN_VERSION "0.2.5.0")
set(COMMONLIB_PLUGIN_LICENSE "MIT")
set(COMMONLIB_PLUGIN_DESCRIPTION "Fallout 4 shader artifact fixes")

project(
	NaNBuster
	VERSION "${COMMONLIB_PLUGIN_VERSION}"
	DESCRIPTION "${COMMONLIB_PLUGIN_DESCRIPTION}"
	LANGUAGES CXX
)

# C12: MSVC MASM (ml64, part of the existing VS toolchain) for the accepted OG
# hook native entry/replay routines.
enable_language(ASM_MASM)

set(NANBUSTER_DISPLAY_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COMMONLIB_RUNTIME_VR OFF)
set(COMMONLIB_RUNTIME_OG ON)
set(COMMONLIB_RUNTIME_NG OFF)
set(COMMONLIB_RUNTIME_AE OFF)
set(COMMONLIB_OPTION_FMT ON)
set(COMMONLIB_OPTION_XBYAK OFF)
set(COMMONLIB_OPTION_INI OFF)
set(COMMONLIB_OPTION_TOML OFF)
set(COMMONLIB_OPTION_JSON OFF)

set(COMMONLIB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/CommonLibF4")
include("${COMMONLIB_ROOT_DIR}/cmake/GenerateXsePluginInfo.cmake")
generate_xse_plugin_info("${COMMONLIB_ROOT_DIR}")

if(NOT TARGET CommonLibF4)
	add_subdirectory("${COMMONLIB_ROOT_DIR}" CommonLibF4 EXCLUDE_FROM_ALL)
endif()

add_library(
	NaNBuster
	SHARED
		src/Infrastructure/Composition.cpp
		src/Infrastructure/ComputeShaderCreationHook.cpp
		src/Infrastructure/EmbeddedShaderPatchRegistry.cpp
		src/Infrastructure/EmbeddedShaderPayload.cpp
		src/Infrastructure/EngineShaderCreator.cpp
		src/Infrastructure/IniConfig.cpp
		src/Infrastructure/PluginEntry.cpp
		src/Infrastructure/PluginLogger.cpp
		"${XSE_PLUGIN_INFO_CPP_TARGET}"
		"${WIN32_VERSION_TARGET}"
)

target_include_directories(
	NaNBuster
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
		"${CMAKE_CURRENT_BINARY_DIR}/include"
)

target_precompile_headers(NaNBuster PRIVATE src/PCH.h)
target_compile_definitions(NaNBuster PRIVATE _UNICODE)
target_compile_definitions(NaNBuster PRIVATE NANBUSTER_DISPLAY_VERSION="${NANBUSTER_DISPLAY_VERSION}")
target_compile_definitions(NaNBuster PRIVATE NANBUSTER_PLUGIN_NAME="${COMMONLIB_PLUGIN_NAME}")
target_compile_definitions(NaNBuster PRIVATE NANBUSTER_PLUGIN_AUTHOR="${COMMONLIB_PLUGIN_AUTHOR}")
target_compile_definitions(NaNBuster PRIVATE NANBUSTER_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
target_compile_definitions(NaNBuster PRIVATE NANBUSTER_VERSION_MINOR=${PROJECT_VERSION_MINOR})
target_compile_definitions(NaNBuster PRIVATE NANBUSTER_VERSION_PATCH=${PROJECT_VERSION_PATCH})
target_compile_definitions(NaNBuster PRIVATE NANBUSTER_VERSION_TWEAK=${PROJECT_VERSION_TWEAK})
# C13: the runtime DLL links the hook component (which brings in
# NaNBuster.Application, NaNBuster.Core, and the MASM native routines) and
# owns process-lifetime composition/installation via Composition/PluginEntry.
target_link_libraries(NaNBuster PRIVATE CommonLibF4::CommonLibF4 NaNBuster.Hook)

find_program(
	NANBUSTER_LLVM_READOBJ
	NAMES llvm-readobj
	HINTS "C:/Program Files/LLVM/bin" "C:/Program Files (x86)/LLVM/bin"
	ENV LLVM_DIR
	PATH_SUFFIXES bin
)
if(NOT NANBUSTER_LLVM_READOBJ)
	message(FATAL_ERROR "llvm-readobj was not found. The NaNBuster export gate requires it; install LLVM or set LLVM_DIR.")
endif()

add_custom_command(
	TARGET NaNBuster
	POST_BUILD
	COMMAND "${CMAKE_COMMAND}"
		"-DNANBUSTER_DLL=$<TARGET_FILE:NaNBuster>"
		"-DNANBUSTER_READOBJ=${NANBUSTER_LLVM_READOBJ}"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyPluginExports.cmake"
	VERBATIM
	COMMENT "Verifying required F4SE plugin exports"
)

add_library(NaNBuster.Core STATIC
	src/Core/DxbcAnisotropicSpecularClampPatcher.cpp
	src/Core/DxbcClampPatcher.cpp
	src/Core/DxbcHashWriter.cpp
	src/Core/DxbcInspector.cpp
	src/Core/DxbcIsotropicSpecularAAPatcher.cpp
	src/Core/DxbcLambertGuardPatcher.cpp
	src/Core/ShaderIdentityMatcher.cpp
	src/Core/ShaderTriage.cpp
	src/Core/TargetShaderMatcher.cpp
)

target_include_directories(NaNBuster.Core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_features(NaNBuster.Core PUBLIC cxx_std_23)

# C8: engine-independent match-and-substitute use case. Consumed by tests and
# by C13 composition through the runtime DLL's NaNBuster.Hook link.
add_library(NaNBuster.Application STATIC
	src/Application/ShaderPatchService.cpp
)

target_link_libraries(NaNBuster.Application PUBLIC NaNBuster.Core)

target_include_directories(NaNBuster.Application PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_features(NaNBuster.Application PUBLIC cxx_std_23)

# C12: OG shader-creation hook component (G5_HOOK_CONTRACT.md site/fingerprint).
# Static library consumed by the C12 tests and linked into the runtime DLL by
# C13; C13 owns composition and installation. Uses CommonLib AV relocation,
# safe-write, and the F4SE near-branch trampoline pool; XBYAK stays off.
# The MASM native routines build as their own component: the pinned
# CommonLibF4 interface C++ options (/WX, /GL, /Zc:*) would otherwise be
# forwarded to ml64 by the VS generator, which rejects them (A4018 as fatal).
add_library(NaNBuster.Hook.Native STATIC
	src/Infrastructure/ComputeShaderCreationHook.asm
	src/Infrastructure/ShaderCreationHook.asm
)

add_library(NaNBuster.Hook STATIC
	src/Infrastructure/ComputeShaderCreationHook.cpp
	src/Infrastructure/ShaderCreationHook.cpp
)

target_link_libraries(NaNBuster.Hook
	PUBLIC
		NaNBuster.Application
		NaNBuster.Hook.Native
	PRIVATE
		CommonLibF4::CommonLibF4
)

target_include_directories(NaNBuster.Hook PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_features(NaNBuster.Hook PUBLIC cxx_std_23)

# C6: offline corrected-shader generation. Build-time only; the runtime DLL
# does not link the tool or the generated payload until C9. Outputs live at
# the approved ${CMAKE_SOURCE_DIR}/build/shaders/PS49B6A21CI0O1 path; the
# NANBUSTER_* variables below are the C9 consumption surface.
add_executable(ShaderPatcher tools/ShaderPatcher/main.cpp)
target_link_libraries(ShaderPatcher PRIVATE NaNBuster.Core)
target_compile_features(ShaderPatcher PRIVATE cxx_std_23)

add_executable(LambertShaderPatcher tools/LambertShaderPatcher/main.cpp)
target_link_libraries(LambertShaderPatcher PRIVATE NaNBuster.Core)
target_compile_features(LambertShaderPatcher PRIVATE cxx_std_23)

add_executable(IsotropicShaderPatcher tools/IsotropicShaderPatcher/main.cpp)
target_link_libraries(IsotropicShaderPatcher PRIVATE NaNBuster.Core)
target_compile_features(IsotropicShaderPatcher PRIVATE cxx_std_23)

add_executable(ShaderTriage tools/ShaderTriage/main.cpp)
target_link_libraries(ShaderTriage PRIVATE NaNBuster.Core)
target_compile_features(ShaderTriage PRIVATE cxx_std_23)

set(NANBUSTER_SHADER_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/build/shaders/PS49B6A21CI0O1")
set(NANBUSTER_ORIGINAL_SHADER_DXBC "${CMAKE_CURRENT_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc")
set(NANBUSTER_PATCHED_DXBC "${NANBUSTER_SHADER_OUTPUT_DIR}/patched.dxbc")
set(NANBUSTER_PATCHED_INC "${NANBUSTER_SHADER_OUTPUT_DIR}/patched.inc")
set(NANBUSTER_PATCHED_ASM "${NANBUSTER_SHADER_OUTPUT_DIR}/patched.asm")

find_program(
	NANBUSTER_FXC_EXE
	NAMES fxc
	HINTS "D:/Windows Kits/10/bin/10.0.26100.0/x64"
	PATH_SUFFIXES bin
)
if(NOT NANBUSTER_FXC_EXE)
	message(FATAL_ERROR "fxc was not found. Corrected-shader disassembly requires the Windows SDK fxc.")
endif()

add_custom_command(
	OUTPUT "${NANBUSTER_PATCHED_DXBC}"
	COMMAND "${CMAKE_COMMAND}" -E make_directory "${NANBUSTER_SHADER_OUTPUT_DIR}"
	COMMAND "$<TARGET_FILE:ShaderPatcher>" "${NANBUSTER_ORIGINAL_SHADER_DXBC}" "${NANBUSTER_PATCHED_DXBC}"
	DEPENDS ShaderPatcher "${NANBUSTER_ORIGINAL_SHADER_DXBC}"
	COMMENT "NaNBuster: generating corrected DXBC"
	VERBATIM
)

add_custom_command(
	OUTPUT "${NANBUSTER_PATCHED_INC}"
	COMMAND "${CMAKE_COMMAND}"
		"-DNANBUSTER_INPUT=${NANBUSTER_PATCHED_DXBC}"
		"-DNANBUSTER_OUTPUT=${NANBUSTER_PATCHED_INC}"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedShader.cmake"
	DEPENDS "${NANBUSTER_PATCHED_DXBC}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedShader.cmake"
	COMMENT "NaNBuster: embedding corrected DXBC as C++ data"
	VERBATIM
)

add_custom_command(
	OUTPUT "${NANBUSTER_PATCHED_ASM}"
	COMMAND "${CMAKE_COMMAND}" -E make_directory "${NANBUSTER_SHADER_OUTPUT_DIR}"
	COMMAND "${NANBUSTER_FXC_EXE}" /dumpbin "${NANBUSTER_PATCHED_DXBC}" /Fx "${NANBUSTER_PATCHED_ASM}"
	DEPENDS "${NANBUSTER_PATCHED_DXBC}"
	COMMENT "NaNBuster: disassembling corrected DXBC with fxc"
	VERBATIM
)

add_custom_target(NaNBuster.GeneratedShader ALL
	DEPENDS "${NANBUSTER_PATCHED_DXBC}" "${NANBUSTER_PATCHED_INC}" "${NANBUSTER_PATCHED_ASM}"
)

set(NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT "${CMAKE_SOURCE_DIR}/build/shaders/LambertDecode")
set(NANBUSTER_LAMBERT_SHADER_HASHES
	"bcc0320d-dee924ba-4d26b206-84d74f3f"
	"8904fc0f-cb497158-117bc46d-4e1cf466"
	"4dc83306-024b9074-9c24d67a-fcdee952"
	"325742ce-45bf17d0-f4879324-3637b6aa"
	"def41f6e-9dbcf63b-4c43cf22-4eea0bb4"
	"600e22f5-e1a79678-b183a8f0-99de971d"
	"d75dfae6-af7b9aa5-1a0374d8-3d186619"
	"6b7aa1fd-0175af05-1bf184b7-22dd8846"
	"4bfc847c-48cbb854-83d96138-488b9bb1"
	"262c37a4-f4966781-271cf8ba-64c06ec1"
	"e73d9a26-0c9afe98-f628f975-0e5f8aef"
	"be21279d-733505d8-43f98a86-09502d65"
)
set(NANBUSTER_LAMBERT_GENERATED_OUTPUTS)
set(NANBUSTER_LAMBERT_INCLUDE_DIRS)
foreach(hash IN LISTS NANBUSTER_LAMBERT_SHADER_HASHES)
	set(outputDir "${NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT}/${hash}")
	set(originalDxbc "${CMAKE_SOURCE_DIR}/references/shaders/LambertDecode/${hash}/original.dxbc")
	set(patchedDxbc "${outputDir}/patched.dxbc")
	set(patchedInc "${outputDir}/patched.inc")
	set(patchedAsm "${outputDir}/patched.asm")
	set(originalNormalizedAsm "${outputDir}/original-normalized.asm")
	string(SUBSTRING "${hash}" 0 8 symbolSuffix)

	add_custom_command(
		OUTPUT "${patchedDxbc}"
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${outputDir}"
		COMMAND "$<TARGET_FILE:LambertShaderPatcher>" "${originalDxbc}" "${patchedDxbc}"
		DEPENDS LambertShaderPatcher "${originalDxbc}"
		COMMENT "NaNBuster: generating guarded Lambert DXBC ${symbolSuffix}"
		VERBATIM
	)
	add_custom_command(
		OUTPUT "${patchedInc}"
		COMMAND "${CMAKE_COMMAND}"
			"-DNANBUSTER_INPUT=${patchedDxbc}"
			"-DNANBUSTER_OUTPUT=${patchedInc}"
			"-DNANBUSTER_SYMBOL=Lambert_${symbolSuffix}"
			-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedShader.cmake"
		DEPENDS "${patchedDxbc}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedShader.cmake"
		COMMENT "NaNBuster: embedding guarded Lambert DXBC ${symbolSuffix}"
		VERBATIM
	)
	add_custom_command(
		OUTPUT "${patchedAsm}"
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${outputDir}"
		COMMAND "${NANBUSTER_FXC_EXE}" /dumpbin "${patchedDxbc}" /Fx "${patchedAsm}"
		DEPENDS "${patchedDxbc}"
		COMMENT "NaNBuster: disassembling guarded Lambert DXBC ${symbolSuffix}"
		VERBATIM
	)
	add_custom_command(
		OUTPUT "${originalNormalizedAsm}"
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${outputDir}"
		COMMAND "${NANBUSTER_FXC_EXE}" /dumpbin "${originalDxbc}" /Fx "${originalNormalizedAsm}"
		DEPENDS "${originalDxbc}"
		COMMENT "NaNBuster: normalizing original Lambert ASM ${symbolSuffix}"
		VERBATIM
	)
	if(hash STREQUAL "8904fc0f-cb497158-117bc46d-4e1cf466" OR
		hash STREQUAL "4dc83306-024b9074-9c24d67a-fcdee952" OR
		hash STREQUAL "325742ce-45bf17d0-f4879324-3637b6aa" OR
		hash STREQUAL "600e22f5-e1a79678-b183a8f0-99de971d" OR
		hash STREQUAL "6b7aa1fd-0175af05-1bf184b7-22dd8846" OR
		hash STREQUAL "262c37a4-f4966781-271cf8ba-64c06ec1" OR
		hash STREQUAL "be21279d-733505d8-43f98a86-09502d65")
		set(anisotropicDxbc "${outputDir}/anisotropic-clamped.dxbc")
		set(anisotropicInc "${outputDir}/anisotropic-clamped.inc")
		set(anisotropicAsm "${outputDir}/anisotropic-clamped.asm")
		set(combinedDxbc "${outputDir}/combined.dxbc")
		set(combinedInc "${outputDir}/combined.inc")
		set(combinedAsm "${outputDir}/combined.asm")

		add_custom_command(
			OUTPUT "${anisotropicDxbc}"
			COMMAND "${CMAKE_COMMAND}" -E make_directory "${outputDir}"
			COMMAND "$<TARGET_FILE:LambertShaderPatcher>" "${originalDxbc}" "${anisotropicDxbc}" --anisotropic-clamp
			DEPENDS LambertShaderPatcher "${originalDxbc}"
			COMMENT "NaNBuster: generating anisotropic-clamped DXBC ${symbolSuffix}"
			VERBATIM
		)
		add_custom_command(
			OUTPUT "${anisotropicInc}"
			COMMAND "${CMAKE_COMMAND}"
				"-DNANBUSTER_INPUT=${anisotropicDxbc}"
				"-DNANBUSTER_OUTPUT=${anisotropicInc}"
				"-DNANBUSTER_SYMBOL=Anisotropic_${symbolSuffix}"
				-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedShader.cmake"
			DEPENDS "${anisotropicDxbc}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedShader.cmake"
			COMMENT "NaNBuster: embedding anisotropic-clamped DXBC ${symbolSuffix}"
			VERBATIM
		)
		add_custom_command(
			OUTPUT "${anisotropicAsm}"
			COMMAND "${CMAKE_COMMAND}" -E make_directory "${outputDir}"
			COMMAND "${NANBUSTER_FXC_EXE}" /dumpbin "${anisotropicDxbc}" /Fx "${anisotropicAsm}"
			DEPENDS "${anisotropicDxbc}"
			COMMENT "NaNBuster: disassembling anisotropic-clamped DXBC ${symbolSuffix}"
			VERBATIM
		)
		add_custom_command(
			OUTPUT "${combinedDxbc}"
			COMMAND "${CMAKE_COMMAND}" -E make_directory "${outputDir}"
			COMMAND "$<TARGET_FILE:LambertShaderPatcher>" "${originalDxbc}" "${combinedDxbc}" --combined-anisotropic-clamp
			DEPENDS LambertShaderPatcher "${originalDxbc}"
			COMMENT "NaNBuster: generating combined guarded DXBC ${symbolSuffix}"
			VERBATIM
		)
		add_custom_command(
			OUTPUT "${combinedInc}"
			COMMAND "${CMAKE_COMMAND}"
				"-DNANBUSTER_INPUT=${combinedDxbc}"
				"-DNANBUSTER_OUTPUT=${combinedInc}"
				"-DNANBUSTER_SYMBOL=Combined_${symbolSuffix}"
				-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedShader.cmake"
			DEPENDS "${combinedDxbc}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedShader.cmake"
			COMMENT "NaNBuster: embedding combined guarded DXBC ${symbolSuffix}"
			VERBATIM
		)
		add_custom_command(
			OUTPUT "${combinedAsm}"
			COMMAND "${CMAKE_COMMAND}" -E make_directory "${outputDir}"
			COMMAND "${NANBUSTER_FXC_EXE}" /dumpbin "${combinedDxbc}" /Fx "${combinedAsm}"
			DEPENDS "${combinedDxbc}"
			COMMENT "NaNBuster: disassembling combined guarded DXBC ${symbolSuffix}"
			VERBATIM
		)
		list(APPEND NANBUSTER_LAMBERT_GENERATED_OUTPUTS
			"${anisotropicDxbc}" "${anisotropicInc}" "${anisotropicAsm}"
			"${combinedDxbc}" "${combinedInc}" "${combinedAsm}")
	endif()
	list(APPEND NANBUSTER_LAMBERT_GENERATED_OUTPUTS "${patchedDxbc}" "${patchedInc}" "${patchedAsm}" "${originalNormalizedAsm}")
	list(APPEND NANBUSTER_LAMBERT_INCLUDE_DIRS "${outputDir}")
endforeach()

add_custom_target(NaNBuster.GeneratedLambertShaders ALL DEPENDS ${NANBUSTER_LAMBERT_GENERATED_OUTPUTS})
add_dependencies(NaNBuster.GeneratedShader NaNBuster.GeneratedLambertShaders)

set(NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/build/shaders/IsotropicSpecularAA/f4b2da50-c279fcd9-bd60ba4f-6a804632")
set(NANBUSTER_ISOTROPIC_ORIGINAL_DXBC "${CMAKE_SOURCE_DIR}/references/shaders/IsotropicSpecularAA/f4b2da50-c279fcd9-bd60ba4f-6a804632/original.dxbc")
set(NANBUSTER_ISOTROPIC_PATCHED_DXBC "${NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR}/patched.dxbc")
set(NANBUSTER_ISOTROPIC_PATCHED_INC "${NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR}/patched.inc")
set(NANBUSTER_ISOTROPIC_PATCHED_ASM "${NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR}/patched.asm")
set(NANBUSTER_ISOTROPIC_ORIGINAL_NORMALIZED_ASM "${NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR}/original-normalized.asm")

add_custom_command(
	OUTPUT "${NANBUSTER_ISOTROPIC_PATCHED_DXBC}"
	COMMAND "${CMAKE_COMMAND}" -E make_directory "${NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR}"
	COMMAND "$<TARGET_FILE:IsotropicShaderPatcher>" "${NANBUSTER_ISOTROPIC_ORIGINAL_DXBC}" "${NANBUSTER_ISOTROPIC_PATCHED_DXBC}"
	DEPENDS IsotropicShaderPatcher "${NANBUSTER_ISOTROPIC_ORIGINAL_DXBC}"
	COMMENT "NaNBuster: generating isotropic specular-AA DXBC"
	VERBATIM
)
add_custom_command(
	OUTPUT "${NANBUSTER_ISOTROPIC_PATCHED_INC}"
	COMMAND "${CMAKE_COMMAND}"
		"-DNANBUSTER_INPUT=${NANBUSTER_ISOTROPIC_PATCHED_DXBC}"
		"-DNANBUSTER_OUTPUT=${NANBUSTER_ISOTROPIC_PATCHED_INC}"
		"-DNANBUSTER_SYMBOL=IsotropicSpecularAA_f4b2da50"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedShader.cmake"
	DEPENDS "${NANBUSTER_ISOTROPIC_PATCHED_DXBC}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedShader.cmake"
	COMMENT "NaNBuster: embedding isotropic specular-AA DXBC"
	VERBATIM
)
add_custom_command(
	OUTPUT "${NANBUSTER_ISOTROPIC_PATCHED_ASM}"
	COMMAND "${CMAKE_COMMAND}" -E make_directory "${NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR}"
	COMMAND "${NANBUSTER_FXC_EXE}" /dumpbin "${NANBUSTER_ISOTROPIC_PATCHED_DXBC}" /Fx "${NANBUSTER_ISOTROPIC_PATCHED_ASM}"
	DEPENDS "${NANBUSTER_ISOTROPIC_PATCHED_DXBC}"
	COMMENT "NaNBuster: disassembling isotropic specular-AA DXBC"
	VERBATIM
)
add_custom_command(
	OUTPUT "${NANBUSTER_ISOTROPIC_ORIGINAL_NORMALIZED_ASM}"
	COMMAND "${CMAKE_COMMAND}" -E make_directory "${NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR}"
	COMMAND "${NANBUSTER_FXC_EXE}" /dumpbin "${NANBUSTER_ISOTROPIC_ORIGINAL_DXBC}" /Fx "${NANBUSTER_ISOTROPIC_ORIGINAL_NORMALIZED_ASM}"
	DEPENDS "${NANBUSTER_ISOTROPIC_ORIGINAL_DXBC}"
	COMMENT "NaNBuster: normalizing original isotropic specular ASM"
	VERBATIM
)
add_custom_target(
	NaNBuster.GeneratedIsotropicSpecularAA ALL
	DEPENDS
		"${NANBUSTER_ISOTROPIC_PATCHED_DXBC}"
		"${NANBUSTER_ISOTROPIC_PATCHED_INC}"
		"${NANBUSTER_ISOTROPIC_PATCHED_ASM}"
		"${NANBUSTER_ISOTROPIC_ORIGINAL_NORMALIZED_ASM}"
)
add_dependencies(NaNBuster.GeneratedShader NaNBuster.GeneratedIsotropicSpecularAA)

# C9: embedded corrected payload. The adapter compiles the generated include
# into the runtime DLL and orders the DLL build after payload generation.
# Header contracts only for C9 itself (ruling D1): the DLL consumes
# NaNBuster.Application at runtime through C13's NaNBuster.Hook link and the
# C13 composition wiring.
target_include_directories(NaNBuster PRIVATE "${NANBUSTER_SHADER_OUTPUT_DIR}" "${NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT}" "${NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR}" "${CMAKE_SOURCE_DIR}/build/shaders")
add_dependencies(NaNBuster NaNBuster.GeneratedShader)

include(CTest)

if(BUILD_TESTING)
	add_subdirectory(tests)
endif()

install(TARGETS NaNBuster RUNTIME DESTINATION F4SE/Plugins)
install(FILES README.md CHANGELOG.md LICENSE THIRD_PARTY_NOTICES.md DESTINATION .)
install(CODE "
	set(NANBUSTER_CONFIG_SOURCE \"${CMAKE_CURRENT_SOURCE_DIR}/assets/config/NaNBuster.ini\")
	set(NANBUSTER_CONFIG_DESTINATION \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/F4SE/Plugins/NaNBuster.ini\")
	include(\"${CMAKE_CURRENT_SOURCE_DIR}/cmake/InstallConfig.cmake\")
")
