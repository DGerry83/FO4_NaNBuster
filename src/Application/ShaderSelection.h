#pragma once

#include "Core/ShaderIdentity.h"

namespace NaNBuster
{
	// Result category for one shader-creation selection. NotTarget: input was
	// not the exact original target and passes through unchanged. Patched:
	// exact target matched and the corrected bytecode was selected.
	// HookUnavailable and CreationFailedOriginal are produced downstream by
	// the hook/composition layers (C12/C13); the selection use case never
	// emits them, but the values stay locked for those consumers.
	enum class PatchOutcome
	{
		NotTarget,
		Patched,
		HookUnavailable,
		CreationFailedOriginal,
	};

	// Immutable outcome of ShaderPatchService::Select. SelectedBytecode is a
	// non-owning view: it addresses either the caller-supplied original bytes
	// (Substitute == false, Outcome == NotTarget) or the payload's corrected
	// storage (Substitute == true, Outcome == Patched). Callers and payload
	// storage must outlive use of the view.
	struct ShaderSelection
	{
		ByteView SelectedBytecode;
		bool Substitute;
		PatchOutcome Outcome;
	};
}
