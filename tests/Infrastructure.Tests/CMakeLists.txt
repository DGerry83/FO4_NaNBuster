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

add_executable(Infrastructure.EmbeddedShaderPayload.Tests
	EmbeddedShaderPayloadTests.cpp
	"${CMAKE_SOURCE_DIR}/src/Infrastructure/EmbeddedShaderPayload.cpp"
)

target_include_directories(
	Infrastructure.EmbeddedShaderPayload.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
		"${NANBUSTER_SHADER_OUTPUT_DIR}"
)

target_compile_definitions(
	Infrastructure.EmbeddedShaderPayload.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
		NANBUSTER_CORRECTED_SHADER_PATH="${NANBUSTER_PATCHED_DXBC}"
)

add_dependencies(Infrastructure.EmbeddedShaderPayload.Tests NaNBuster.GeneratedShader)

add_test(NAME Infrastructure.EmbeddedShaderPayload COMMAND Infrastructure.EmbeddedShaderPayload.Tests)

add_executable(Infrastructure.EngineShaderCreator.Tests
	EngineShaderCreatorTests.cpp
	"${CMAKE_SOURCE_DIR}/src/Infrastructure/EngineShaderCreator.cpp"
)

target_include_directories(
	Infrastructure.EngineShaderCreator.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
)

add_test(NAME Infrastructure.EngineShaderCreator COMMAND Infrastructure.EngineShaderCreator.Tests)

add_executable(Infrastructure.ShaderCreationHook.Tests
	ShaderCreationHookTests.cpp
)

target_link_libraries(Infrastructure.ShaderCreationHook.Tests PRIVATE NaNBuster.Hook)

target_include_directories(
	Infrastructure.ShaderCreationHook.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
)

target_compile_definitions(
	Infrastructure.ShaderCreationHook.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
		NANBUSTER_CORRECTED_SHADER_PATH="${NANBUSTER_PATCHED_DXBC}"
)

add_dependencies(Infrastructure.ShaderCreationHook.Tests NaNBuster.GeneratedShader)

add_test(NAME Infrastructure.ShaderCreationHook COMMAND Infrastructure.ShaderCreationHook.Tests)


add_executable(Infrastructure.LambertShaderValidation.Tests LambertShaderValidationTests.cpp)

target_link_libraries(Infrastructure.LambertShaderValidation.Tests PRIVATE NaNBuster.Core d3d11)

target_include_directories(
	Infrastructure.LambertShaderValidation.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
		"${NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT}"
)

target_compile_definitions(
	Infrastructure.LambertShaderValidation.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
		NANBUSTER_LAMBERT_REFERENCE_ROOT="${CMAKE_SOURCE_DIR}/references/shaders/LambertDecode"
		NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT="${NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT}"
)

add_dependencies(Infrastructure.LambertShaderValidation.Tests NaNBuster.GeneratedLambertShaders)

add_test(NAME Infrastructure.LambertShaderValidation COMMAND Infrastructure.LambertShaderValidation.Tests)


add_executable(Infrastructure.AnisotropicSpecularClampValidation.Tests AnisotropicSpecularClampValidationTests.cpp)

target_link_libraries(Infrastructure.AnisotropicSpecularClampValidation.Tests PRIVATE NaNBuster.Core d3d11)

target_include_directories(
	Infrastructure.AnisotropicSpecularClampValidation.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
		"${NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT}"
)

target_compile_definitions(
	Infrastructure.AnisotropicSpecularClampValidation.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
		NANBUSTER_LAMBERT_REFERENCE_ROOT="${CMAKE_SOURCE_DIR}/references/shaders/LambertDecode"
		NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT="${NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT}"
)

add_dependencies(Infrastructure.AnisotropicSpecularClampValidation.Tests NaNBuster.GeneratedLambertShaders)

add_test(NAME Infrastructure.AnisotropicSpecularClampValidation COMMAND Infrastructure.AnisotropicSpecularClampValidation.Tests)


add_executable(Infrastructure.IsotropicSpecularAAValidation.Tests IsotropicSpecularAAValidationTests.cpp)

target_link_libraries(Infrastructure.IsotropicSpecularAAValidation.Tests PRIVATE NaNBuster.Core d3d11)

target_include_directories(
	Infrastructure.IsotropicSpecularAAValidation.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
		"${NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR}"
)

target_compile_definitions(
	Infrastructure.IsotropicSpecularAAValidation.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
		NANBUSTER_ISOTROPIC_ORIGINAL_PATH="${CMAKE_SOURCE_DIR}/references/shaders/IsotropicSpecularAA/f4b2da50-c279fcd9-bd60ba4f-6a804632/original.dxbc"
		NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR="${NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR}"
)

add_dependencies(Infrastructure.IsotropicSpecularAAValidation.Tests NaNBuster.GeneratedIsotropicSpecularAA)

add_test(NAME Infrastructure.IsotropicSpecularAAValidation COMMAND Infrastructure.IsotropicSpecularAAValidation.Tests)


add_executable(Infrastructure.EmbeddedShaderPatchRegistry.Tests
	EmbeddedShaderPatchRegistryTests.cpp
	"${CMAKE_SOURCE_DIR}/src/Infrastructure/EmbeddedShaderPatchRegistry.cpp"
)

target_link_libraries(Infrastructure.EmbeddedShaderPatchRegistry.Tests PRIVATE NaNBuster.Application)

target_include_directories(
	Infrastructure.EmbeddedShaderPatchRegistry.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
		"${NANBUSTER_SHADER_OUTPUT_DIR}"
		"${NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT}"
		"${CMAKE_SOURCE_DIR}/build/shaders"
)

target_compile_definitions(
	Infrastructure.EmbeddedShaderPatchRegistry.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
		NANBUSTER_CORRECTED_SHADER_PATH="${NANBUSTER_PATCHED_DXBC}"
		NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT="${NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT}"
		NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR="${NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR}"
)

add_dependencies(Infrastructure.EmbeddedShaderPatchRegistry.Tests NaNBuster.GeneratedShader)

add_test(NAME Infrastructure.EmbeddedShaderPatchRegistry COMMAND Infrastructure.EmbeddedShaderPatchRegistry.Tests)


add_executable(Infrastructure.IniConfig.Tests
	IniConfigTests.cpp
	"${CMAKE_SOURCE_DIR}/src/Infrastructure/IniConfig.cpp"
)

target_link_libraries(Infrastructure.IniConfig.Tests PRIVATE NaNBuster.Application)

target_include_directories(
	Infrastructure.IniConfig.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
)

target_compile_definitions(
	Infrastructure.IniConfig.Tests
	PRIVATE
		NANBUSTER_DEFAULT_INI_PATH="${CMAKE_SOURCE_DIR}/assets/config/NaNBuster.ini"
)

add_test(NAME Infrastructure.IniConfig COMMAND Infrastructure.IniConfig.Tests)


add_executable(Infrastructure.ComputeShaderCreationHook.Tests
	ComputeShaderCreationHookTests.cpp
	"${CMAKE_SOURCE_DIR}/src/Infrastructure/EmbeddedShaderPatchRegistry.cpp"
)

target_link_libraries(Infrastructure.ComputeShaderCreationHook.Tests PRIVATE NaNBuster.Hook)

target_include_directories(
	Infrastructure.ComputeShaderCreationHook.Tests
	PRIVATE
		"${CMAKE_SOURCE_DIR}/src"
		"${CMAKE_SOURCE_DIR}/tests"
		"${NANBUSTER_SHADER_OUTPUT_DIR}"
		"${NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT}"
		"${CMAKE_SOURCE_DIR}/build/shaders"
)

target_compile_definitions(
	Infrastructure.ComputeShaderCreationHook.Tests
	PRIVATE
		NANBUSTER_ORIGINAL_SHADER_PATH="${CMAKE_SOURCE_DIR}/references/shaders/PS49B6A21CI0O1/original.dxbc"
		NANBUSTER_CORRECTED_SHADER_PATH="${NANBUSTER_PATCHED_DXBC}"
		NANBUSTER_LAMBERT_REFERENCE_ROOT="${CMAKE_SOURCE_DIR}/references/shaders/LambertDecode"
		NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT="${NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT}"
)

add_dependencies(Infrastructure.ComputeShaderCreationHook.Tests NaNBuster.GeneratedShader)

add_test(NAME Infrastructure.ComputeShaderCreationHook COMMAND Infrastructure.ComputeShaderCreationHook.Tests)
