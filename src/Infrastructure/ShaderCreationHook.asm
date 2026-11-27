; -----------------------------------------------------------------------------
; NaNBuster - OG/AE shader-creation hook: native entry + original-call replay.
; Evidence: OG G5_HOOK_CONTRACT.md §4-§7 and AE_HOOK_CONTRACT.md §4-§7.
;
; OG site:    0x141d10e82, owner ID 957517, offset 0x142, 34-byte fingerprint.
; AE site:    0x14181defe, owner ID 2276951, offset 0x12e, 30-byte fingerprint.
; Detour:     7 bytes (E9 rel32 + 2 NOP) at either active-runtime site.
; Resume:     OG 0x141d10ea4 / AE 0x14181df1c (site + fingerprint size).
; Device:     ID3D11Device* global, OG ID 1454788 / AE ID 2704430.
;
; HookCreationContext layout (duplicated as C++ static_asserts in
; ShaderCreationHook.h; both sides must agree):
;   +0  OgCreationContext.device           (ID3D11Device*)
;   +8  OgCreationContext.engineWrapper    (BSGraphics::PixelShader*)
;   +16 OgCreationContext.originalBytecode (rbx)
;   +24 OgCreationContext.originalSize     ([siteStack+0x60])
;   +32 OgCreationContext.outShaderSlot    (&wrapper+8, lea rdi+8 at entry)
;   +40 siteStack                          (captured engine rsp at the site)
;   sizeof = 48
;
; Native entry frame (parent review D7): the Win64 callee shadow area
; (+0x00..+0x1F) is reserved ahead of the context so the C++ dispatch can
; never overwrite context fields through its mandatory 32-byte home area.
;   +0x00..+0x1F  callee shadow (never written by the entry itself)
;   +0x20..+0x4F  HookCreationContext
;   sizeof = 80 (0x50), a multiple of 16, so call-site alignment is preserved.
; -----------------------------------------------------------------------------

OPTION CASEMAP:NONE

ENTRY_FRAME_SIZE    EQU 050h    ; 0x20 shadow + 0x30 context
ENTRY_SHADOW_SIZE   EQU 020h    ; Win64 callee shadow area
ENTRY_CONTEXT_BASE  EQU 020h    ; HookCreationContext base inside the frame

HCTX_DEVICE         EQU 0
HCTX_ENGINE_WRAPPER EQU 8
HCTX_ORIGINAL_BYTES EQU 16
HCTX_ORIGINAL_SIZE  EQU 24
HCTX_OUT_SLOT       EQU 32
HCTX_SITE_STACK     EQU 40
HCTX_SIZE           EQU 48

; Engine-frame slots relative to the captured site stack pointer.
SITE_ORIGINAL_SIZE_SLOT EQU 060h    ; [siteStack+0x60] = original size (entry only)
SITE_OUT_ARG_SLOT       EQU 020h    ; [siteStack+0x20] = &wrapper+8 out-arg home

; ID3D11Device vtable slot for CreatePixelShader (G5 §5.1, SDK 10.0.26100.0).
CREATE_PIXEL_SHADER_SLOT EQU 078h

; NaNBuster_HookState element indices (element type = QWORD; defined in
; ShaderCreationHook.cpp and static-asserted there).
NTS_DEVICE_GLOBAL_VA EQU 0*8        ; resolved VA of the ID3D11Device* global
NTS_RESUME_VA        EQU 1*8        ; resolved resume VA (site + 34)
NTS_HOOK_INSTANCE    EQU 2*8        ; owning ShaderCreationHook (dispatch thunk)

; Compile-time layout guards (mirror of the C++ static_asserts).
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

EXTERN NaNBuster_HookState:QWORD
EXTERN NaNBuster_DispatchShaderCreation:PROC

.code

; -----------------------------------------------------------------------------
; NaNBuster_HookEntry
;   Engine detour target, jumped to from 0x141d10e82 (never called).
;   Live state on arrival (G5 §4/§5): rsp = captured site stack S
;   (16-byte aligned at the call site), rbx = raw original DXBC buffer,
;   rdi = engine wrapper, rsi = stream cursor (untouched).
;
;   Frame: ENTRY_FRAME_SIZE (0x50) bytes — 0x20 of Win64 callee shadow at
;   +0x00..+0x1F, then HookCreationContext at +0x20..+0x4F. Original-site slots
;   are addressed relative to S = rsp + ENTRY_FRAME_SIZE. Calls the C++
;   dispatch thunk (void ABI: rcx = bytes, rdx = size, r8 = &context), then
;   resumes the engine at the verified post-call VA with rax = engine wrapper.
;   Only volatile registers are clobbered; rbx/rdi/rsi survive by construction
;   and through the C++ ABI; the stack is exactly balanced.
; -----------------------------------------------------------------------------
NaNBuster_HookEntry PROC
	sub rsp, ENTRY_FRAME_SIZE                           ; shadow + context; rsp stays 16-aligned
	mov rax, [NaNBuster_HookState + NTS_DEVICE_GLOBAL_VA]
	mov rax, [rax]                                      ; ID3D11Device* (resolved global read)
	mov [rsp + ENTRY_CONTEXT_BASE + HCTX_DEVICE], rax
	mov [rsp + ENTRY_CONTEXT_BASE + HCTX_ENGINE_WRAPPER], rdi     ; engine wrapper
	mov [rsp + ENTRY_CONTEXT_BASE + HCTX_ORIGINAL_BYTES], rbx     ; raw original DXBC buffer
	mov eax, [rsp + ENTRY_FRAME_SIZE + SITE_ORIGINAL_SIZE_SLOT]   ; [S+0x60] original size
	mov [rsp + ENTRY_CONTEXT_BASE + HCTX_ORIGINAL_SIZE], rax      ; zero-extends; padding stays 0
	lea rax, [rdi + 8]                                            ; &wrapper+8 (mirrors original lea at 0x141d10e8e)
	mov [rsp + ENTRY_CONTEXT_BASE + HCTX_OUT_SLOT], rax
	lea rax, [rsp + ENTRY_FRAME_SIZE]                             ; S = captured site stack
	mov [rsp + ENTRY_CONTEXT_BASE + HCTX_SITE_STACK], rax
	mov rcx, rbx                                        ; original bytes view (data)
	mov edx, [rsp + ENTRY_FRAME_SIZE + SITE_ORIGINAL_SIZE_SLOT]   ; original bytes view (size)
	lea r8, [rsp + ENTRY_CONTEXT_BASE]                ; &context
	call NaNBuster_DispatchShaderCreation
	mov rax, rdi                                        ; resume: rax = engine wrapper
	add rsp, ENTRY_FRAME_SIZE
	mov r11, [NaNBuster_HookState + NTS_RESUME_VA]      ; verified resume VA
	jmp r11
NaNBuster_HookEntry ENDP

; -----------------------------------------------------------------------------
; NaNBuster_ReplayOriginalCreation
;   Semantic MASM replay of the accepted 34-byte device-call sequence (G5 §4),
;   invoked once per C11 Create through HookReplayCreationCallback.
;     rcx = selected data, rdx = selected size, r8 = &HookCreationContext
;
;   Selected ByteView data AND size drive the call; the stale original-size
;   stack slot is never read for corrected bytes. The out-slot value is the
;   entry-computed &wrapper+8 (context.engine.outShaderSlot, lea rdi+8) and is written to
;   this call's 5th-argument home ([rsp+0x20], 16-byte call-site alignment) and
;   mirrored to the accepted engine-frame slot [siteStack+0x20], which the
;   original instruction itself wrote. The device global is re-read per call.
;   Returns the post-call HRESULT/eax to C11. Clobbers volatile registers only.
; -----------------------------------------------------------------------------
NaNBuster_ReplayOriginalCreation PROC
	sub rsp, 028h                              ; 5th-arg home; rsp: 8 -> 0 (mod 16)
	mov r9, [r8 + HCTX_SITE_STACK]             ; captured engine stack S
	mov r10, [NaNBuster_HookState + NTS_DEVICE_GLOBAL_VA]
	mov r10, [r10]                             ; ID3D11Device*
	mov rax, [r8 + HCTX_OUT_SLOT]              ; &wrapper+8 (entry-computed lea rdi+8)
	mov [rsp + SITE_OUT_ARG_SLOT], rax         ; 5th argument for this call
	mov [r9 + SITE_OUT_ARG_SLOT], rax          ; mirror the accepted frame write
	mov rax, [r10]                             ; vtable
	xor r9d, r9d                               ; pClassLinkage = NULL (arg4)
	mov r8, rdx                                ; selected size (arg3)
	mov rdx, rcx                               ; selected bytes (arg2)
	mov rcx, r10                               ; device (arg1)
	call qword ptr [rax + CREATE_PIXEL_SHADER_SLOT]
	add rsp, 028h
	ret                                        ; eax = HRESULT
NaNBuster_ReplayOriginalCreation ENDP

END
