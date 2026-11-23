OPTION CASEMAP:NONE

ENTRY_FRAME_SIZE    EQU 050h
ENTRY_SHADOW_SIZE   EQU 020h
ENTRY_CONTEXT_BASE  EQU 020h

HCTX_DEVICE         EQU 0
HCTX_ENGINE_WRAPPER EQU 8
HCTX_ORIGINAL_BYTES EQU 16
HCTX_ORIGINAL_SIZE  EQU 24
HCTX_OUT_SLOT       EQU 32
HCTX_SITE_STACK     EQU 40
HCTX_SIZE           EQU 48

SITE_ORIGINAL_SIZE_SLOT EQU 058h
SITE_OUT_ARG_SLOT       EQU 020h
CREATE_COMPUTE_SHADER_SLOT EQU 090h

NTS_DEVICE_GLOBAL_VA EQU 0*8
NTS_RESUME_VA        EQU 1*8
NTS_HOOK_INSTANCE    EQU 2*8

.errnz ENTRY_FRAME_SIZE - 050h
.errnz ENTRY_SHADOW_SIZE - 020h
.errnz ENTRY_CONTEXT_BASE - 020h
.errnz ENTRY_CONTEXT_BASE + HCTX_SIZE - ENTRY_FRAME_SIZE
.errnz HCTX_DEVICE - 0
.errnz HCTX_ENGINE_WRAPPER - 8
.errnz HCTX_ORIGINAL_BYTES - 16
.errnz HCTX_ORIGINAL_SIZE - 24
.errnz HCTX_OUT_SLOT - 32
.errnz HCTX_SITE_STACK - 40
.errnz HCTX_SIZE - 48
.errnz NTS_DEVICE_GLOBAL_VA - 0
.errnz NTS_RESUME_VA - 8
.errnz NTS_HOOK_INSTANCE - 16

EXTERN NaNBuster_ComputeHookState:QWORD
EXTERN NaNBuster_ComputeDispatchShaderCreation:PROC

.code

NaNBuster_ComputeHookEntry PROC
	sub rsp, ENTRY_FRAME_SIZE
	mov rax, [NaNBuster_ComputeHookState + NTS_DEVICE_GLOBAL_VA]
	mov rax, [rax]
	mov [rsp + ENTRY_CONTEXT_BASE + HCTX_DEVICE], rax
	mov [rsp + ENTRY_CONTEXT_BASE + HCTX_ENGINE_WRAPPER], rbx
	lea rax, [rbx + 088h]
	mov [rsp + ENTRY_CONTEXT_BASE + HCTX_ORIGINAL_BYTES], rax
	mov eax, [rsp + ENTRY_FRAME_SIZE + SITE_ORIGINAL_SIZE_SLOT]
	mov [rsp + ENTRY_CONTEXT_BASE + HCTX_ORIGINAL_SIZE], rax
	lea rax, [rbx + 8]
	mov [rsp + ENTRY_CONTEXT_BASE + HCTX_OUT_SLOT], rax
	lea rax, [rsp + ENTRY_FRAME_SIZE]
	mov [rsp + ENTRY_CONTEXT_BASE + HCTX_SITE_STACK], rax
	lea rcx, [rbx + 088h]
	mov edx, [rsp + ENTRY_FRAME_SIZE + SITE_ORIGINAL_SIZE_SLOT]
	lea r8, [rsp + ENTRY_CONTEXT_BASE]
	call NaNBuster_ComputeDispatchShaderCreation
	mov rax, rbx
	add rsp, ENTRY_FRAME_SIZE
	mov r11, [NaNBuster_ComputeHookState + NTS_RESUME_VA]
	jmp r11
NaNBuster_ComputeHookEntry ENDP

NaNBuster_ComputeReplayOriginalCreation PROC
	sub rsp, 028h
	mov r9, [r8 + HCTX_SITE_STACK]
	mov r10, [NaNBuster_ComputeHookState + NTS_DEVICE_GLOBAL_VA]
	mov r10, [r10]
	mov rax, [r8 + HCTX_OUT_SLOT]
	mov [rsp + SITE_OUT_ARG_SLOT], rax
	mov [r9 + SITE_OUT_ARG_SLOT], rax
	mov rax, [r10]
	xor r9d, r9d
	mov r8, rdx
	mov rdx, rcx
	mov rcx, r10
	call qword ptr [rax + CREATE_COMPUTE_SHADER_SLOT]
	add rsp, 028h
	ret
NaNBuster_ComputeReplayOriginalCreation ENDP

END
