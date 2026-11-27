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

add_executable(Core.ShaderTriage.Tests ShaderTriageTests.cpp)

target_link_libraries(Core.ShaderTriage.Tests PRIVATE NaNBuster.Core)

target_include_directories(
	Core.ShaderTriage.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
)

target_compile_definitions(
	Core.ShaderTriage.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_ASM_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.asm"
		NANBUSTER_CORRECTED_ASM_PATH="${NANBUSTER_PATCHED_ASM}"
)

add_dependencies(Core.ShaderTriage.Tests NaNBuster.GeneratedShader)

add_test(NAME Core.ShaderTriage COMMAND Core.ShaderTriage.Tests)


add_executable(Core.LambertShaderIdentity.Tests LambertShaderIdentityTests.cpp)

target_link_libraries(Core.LambertShaderIdentity.Tests PRIVATE NaNBuster.Core)

target_include_directories(
	Core.LambertShaderIdentity.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
)

target_compile_definitions(
	Core.LambertShaderIdentity.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
		NANBUSTER_LAMBERT_REFERENCE_ROOT="${CMAKE_SOURCE_DIR}/references/shaders/LambertDecode"
)

add_test(NAME Core.LambertShaderIdentity COMMAND Core.LambertShaderIdentity.Tests)


add_executable(Core.DxbcLambertGuardPatcher.Tests DxbcLambertGuardPatcherTests.cpp)

target_link_libraries(Core.DxbcLambertGuardPatcher.Tests PRIVATE NaNBuster.Core)

target_include_directories(
	Core.DxbcLambertGuardPatcher.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
)

target_compile_definitions(
	Core.DxbcLambertGuardPatcher.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
		NANBUSTER_LAMBERT_REFERENCE_ROOT="${CMAKE_SOURCE_DIR}/references/shaders/LambertDecode"
)

add_test(NAME Core.DxbcLambertGuardPatcher COMMAND Core.DxbcLambertGuardPatcher.Tests)


add_executable(Core.DxbcAnisotropicSpecularClampPatcher.Tests DxbcAnisotropicSpecularClampPatcherTests.cpp)

target_link_libraries(Core.DxbcAnisotropicSpecularClampPatcher.Tests PRIVATE NaNBuster.Core)

target_include_directories(
	Core.DxbcAnisotropicSpecularClampPatcher.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
)

target_compile_definitions(
	Core.DxbcAnisotropicSpecularClampPatcher.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
		NANBUSTER_LAMBERT_REFERENCE_ROOT="${CMAKE_SOURCE_DIR}/references/shaders/LambertDecode"
)

add_test(NAME Core.DxbcAnisotropicSpecularClampPatcher COMMAND Core.DxbcAnisotropicSpecularClampPatcher.Tests)


add_executable(Core.DxbcIsotropicSpecularAAPatcher.Tests DxbcIsotropicSpecularAAPatcherTests.cpp)

target_link_libraries(Core.DxbcIsotropicSpecularAAPatcher.Tests PRIVATE NaNBuster.Core)

target_include_directories(
	Core.DxbcIsotropicSpecularAAPatcher.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
)

target_compile_definitions(
	Core.DxbcIsotropicSpecularAAPatcher.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
		NANBUSTER_ISOTROPIC_ORIGINAL_PATH="${CMAKE_SOURCE_DIR}/references/shaders/IsotropicSpecularAA/f4b2da50-c279fcd9-bd60ba4f-6a804632/original.dxbc"
)

add_test(NAME Core.DxbcIsotropicSpecularAAPatcher COMMAND Core.DxbcIsotropicSpecularAAPatcher.Tests)


add_executable(Core.AEShaderVariantIdentity.Tests AEShaderVariantIdentityTests.cpp)

target_link_libraries(Core.AEShaderVariantIdentity.Tests PRIVATE NaNBuster.Core)

target_include_directories(
	Core.AEShaderVariantIdentity.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
)

target_compile_definitions(
	Core.AEShaderVariantIdentity.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
		NANBUSTER_AE_REFERENCE_ROOT="${CMAKE_SOURCE_DIR}/references/shaders/AEShaderVariants"
)

add_test(NAME Core.AEShaderVariantIdentity COMMAND Core.AEShaderVariantIdentity.Tests)
