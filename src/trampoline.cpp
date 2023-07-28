#include "trampoline.h"

#include <Windows.h>

#include "defines.h"

_START_WINHOOKUPP_NM_

BOOL TrampolineHook::CreateTrampolineFunction() noexcept
{
	return true;
}

_END_WINHOOKUPP_NM_