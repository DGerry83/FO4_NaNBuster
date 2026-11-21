add_executable(Core.ShaderIdentity.Tests ShaderIdentityTests.cpp)

target_include_directories(
	Core.ShaderIdentity.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
)

target_compile_definitions(
	Core.ShaderIdentity.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
)

add_test(NAME Core.ShaderIdentity COMMAND Core.ShaderIdentity.Tests)

add_executable(Core.DxbcInspector.Tests DxbcInspectorTests.cpp)

target_link_libraries(Core.DxbcInspector.Tests PRIVATE NaNBuster.Core)

target_include_directories(
	Core.DxbcInspector.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
)

target_compile_definitions(
	Core.DxbcInspector.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
)

add_test(NAME Core.DxbcInspector COMMAND Core.DxbcInspector.Tests)

add_executable(Core.DxbcClampPatcher.Tests DxbcClampPatcherTests.cpp)

target_link_libraries(Core.DxbcClampPatcher.Tests PRIVATE NaNBuster.Core)

target_include_directories(
	Core.DxbcClampPatcher.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
)

target_compile_definitions(
	Core.DxbcClampPatcher.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
)

add_test(NAME Core.DxbcClampPatcher COMMAND Core.DxbcClampPatcher.Tests)

add_executable(Core.DxbcHashWriter.Tests DxbcHashWriterTests.cpp)

target_link_libraries(Core.DxbcHashWriter.Tests PRIVATE NaNBuster.Core)

target_include_directories(
	Core.DxbcHashWriter.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
)

target_compile_definitions(
	Core.DxbcHashWriter.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
)

add_test(NAME Core.DxbcHashWriter COMMAND Core.DxbcHashWriter.Tests)

add_executable(Core.TargetShaderMatcher.Tests TargetShaderMatcherTests.cpp)

target_link_libraries(Core.TargetShaderMatcher.Tests PRIVATE NaNBuster.Core)

target_include_directories(
	Core.TargetShaderMatcher.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
)

target_compile_definitions(
	Core.TargetShaderMatcher.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
		NANBUSTER_CORRECTED_SHADER_PATH="${NANBUSTER_PATCHED_DXBC}"
)

add_dependencies(Core.TargetShaderMatcher.Tests NaNBuster.GeneratedShader)

add_test(NAME Core.TargetShaderMatcher COMMAND Core.TargetShaderMatcher.Tests)
