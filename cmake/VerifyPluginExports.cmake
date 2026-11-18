# VerifyPluginExports.cmake — build-time F4SE export gate for the NaNBuster DLL.
#
# Expected -D variables:
#   NANBUSTER_DLL     — full path to the built NaNBuster DLL
#   NANBUSTER_READOBJ — full path to the llvm-readobj executable
#
# Required exports (build fails when missing): F4SEPlugin_Query, F4SEPlugin_Load,
# F4SEPlugin_Version. OG F4SE itself loads via Query/Load, but the pinned
# CommonLibF4 AV resolves F4SEPlugin_Version internally during F4SE::Init
# (PluginVersionData::GetSingleton), so an OG-only DLL without it fails at load.
# Inspected and reported only (not required): F4SEPlugin_Preload.

if(NOT DEFINED NANBUSTER_DLL OR "${NANBUSTER_DLL}" STREQUAL "" OR NOT EXISTS "${NANBUSTER_DLL}")
	message(FATAL_ERROR "VerifyPluginExports: built DLL not found: '${NANBUSTER_DLL}'")
endif()

if(NOT DEFINED NANBUSTER_READOBJ OR "${NANBUSTER_READOBJ}" STREQUAL "" OR NOT EXISTS "${NANBUSTER_READOBJ}")
	message(FATAL_ERROR "VerifyPluginExports: llvm-readobj not found: '${NANBUSTER_READOBJ}'")
endif()

execute_process(
	COMMAND "${NANBUSTER_READOBJ}" --coff-exports "${NANBUSTER_DLL}"
	RESULT_VARIABLE _readobj_result
	OUTPUT_VARIABLE _readobj_stdout
	ERROR_VARIABLE _readobj_stderr
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT _readobj_result EQUAL 0)
	message(FATAL_ERROR "VerifyPluginExports: llvm-readobj failed on '${NANBUSTER_DLL}' (exit ${_readobj_result}): ${_readobj_stderr}")
endif()

foreach(_required_export IN ITEMS F4SEPlugin_Query F4SEPlugin_Load F4SEPlugin_Version)
	string(FIND "${_readobj_stdout}" "Name: ${_required_export}" _export_pos)
	if(_export_pos EQUAL -1)
		message(FATAL_ERROR "VerifyPluginExports: required export '${_required_export}' is missing from '${NANBUSTER_DLL}'.")
	endif()
endforeach()

string(FIND "${_readobj_stdout}" "Name: F4SEPlugin_Preload" _preload_pos)
if(_preload_pos EQUAL -1)
	set(_preload_status "absent (inspected, not required)")
else()
	set(_preload_status "present (inspected, not required)")
endif()

message(STATUS "VerifyPluginExports: OK — ${NANBUSTER_DLL} exports F4SEPlugin_Query, F4SEPlugin_Load, and F4SEPlugin_Version (required); F4SEPlugin_Preload ${_preload_status}.")
