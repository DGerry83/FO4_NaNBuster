cmake_minimum_required(VERSION 4.3.0)

set(COMMONLIB_PLUGIN_NAME "NaNBuster")
set(COMMONLIB_PLUGIN_AUTHOR "DGerry83")
set(COMMONLIB_PLUGIN_VERSION "0.1.0.0")
set(COMMONLIB_PLUGIN_LICENSE "MIT")
set(COMMONLIB_PLUGIN_DESCRIPTION "Fallout 4 pixel-shader NaN fix")

project(
	NaNBuster
	VERSION "${COMMONLIB_PLUGIN_VERSION}"
	DESCRIPTION "${COMMONLIB_PLUGIN_DESCRIPTION}"
	LANGUAGES CXX
)

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
target_link_libraries(NaNBuster PRIVATE CommonLibF4::CommonLibF4)

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
	src/Core/DxbcClampPatcher.cpp
	src/Core/DxbcHashWriter.cpp
	src/Core/DxbcInspector.cpp
	src/Core/TargetShaderMatcher.cpp
)

target_include_directories(NaNBuster.Core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_features(NaNBuster.Core PUBLIC cxx_std_23)

# C8: engine-independent match-and-substitute use case. Consumed by tests now
# and by C9/C13 composition; the runtime DLL does not link it yet.
add_library(NaNBuster.Application STATIC
	src/Application/ShaderPatchService.cpp
)

target_link_libraries(NaNBuster.Application PUBLIC NaNBuster.Core)

target_include_directories(NaNBuster.Application PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_features(NaNBuster.Application PUBLIC cxx_std_23)

# C6: offline corrected-shader generation. Build-time only; the runtime DLL
# does not link the tool or the generated payload until C9. Outputs live at
# the approved ${CMAKE_SOURCE_DIR}/build/shaders/PS49B6A21CI0O1 path; the
# NANBUSTER_* variables below are the C9 consumption surface.
add_executable(ShaderPatcher tools/ShaderPatcher/main.cpp)
target_link_libraries(ShaderPatcher PRIVATE NaNBuster.Core)
target_compile_features(ShaderPatcher PRIVATE cxx_std_23)

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

include(CTest)

if(BUILD_TESTING)
	add_subdirectory(tests)
endif()

install(TARGETS NaNBuster RUNTIME DESTINATION F4SE/Plugins)
install(FILES README.md CHANGELOG.md LICENSE THIRD_PARTY_NOTICES.md DESTINATION .)
