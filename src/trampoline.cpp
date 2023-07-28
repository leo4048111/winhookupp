#include "trampoline.h"

#include <Windows.h>

#include "defines.h"

_START_WINHOOKUPP_NM_

BOOL Trampoline::CreateTrampolineFunction() noexcept
{
	return true;
}

BOOL Trampoline::Enable(LPVOID target, LPVOID detour) noexcept
{
	// TODO: implementation
	return true;
}

BOOL Trampoline::Disable() noexcept
{
	// TODO: implementation
	return true;
}

_END_WINHOOKUPP_NM_