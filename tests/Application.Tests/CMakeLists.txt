add_executable(Application.ShaderPatchService.Tests ShaderPatchServiceTests.cpp)

target_link_libraries(Application.ShaderPatchService.Tests PRIVATE NaNBuster.Application)

target_include_directories(
	Application.ShaderPatchService.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
)

target_compile_definitions(
	Application.ShaderPatchService.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
		NANBUSTER_CORRECTED_SHADER_PATH="${NANBUSTER_PATCHED_DXBC}"
)

add_dependencies(Application.ShaderPatchService.Tests NaNBuster.GeneratedShader)

add_test(NAME Application.ShaderPatchService COMMAND Application.ShaderPatchService.Tests)
