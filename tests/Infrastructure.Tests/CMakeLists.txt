add_executable(Infrastructure.CorrectedShaderValidation.Tests CorrectedShaderValidationTests.cpp)

target_link_libraries(Infrastructure.CorrectedShaderValidation.Tests PRIVATE NaNBuster.Core d3d11)

target_include_directories(
	Infrastructure.CorrectedShaderValidation.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
		"${NANBUSTER_SHADER_OUTPUT_DIR}"
)

target_compile_definitions(
	Infrastructure.CorrectedShaderValidation.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
		NANBUSTER_ORIGINAL_ASM_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.asm"
		NANBUSTER_CORRECTED_SHADER_PATH="${NANBUSTER_PATCHED_DXBC}"
		NANBUSTER_PATCHED_ASM_PATH="${NANBUSTER_PATCHED_ASM}"
)

add_dependencies(Infrastructure.CorrectedShaderValidation.Tests NaNBuster.GeneratedShader)

add_test(NAME Infrastructure.CorrectedShaderValidation COMMAND Infrastructure.CorrectedShaderValidation.Tests)
