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

include(CTest)

install(TARGETS NaNBuster RUNTIME DESTINATION F4SE/Plugins)
install(FILES README.md CHANGELOG.md LICENSE THIRD_PARTY_NOTICES.md DESTINATION .)
