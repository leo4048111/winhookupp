#pragma once

#include "defines.h"

#include <cstdint>

_START_WINHOOKUPP_NM_

class TrampolineHook
{
public:
	TrampolineHook(uintptr_t target, uintptr_t detour, size_t size);
	~TrampolineHook();

	size_t GetSize() const;
	uintptr_t GetTarget() const;
	uintptr_t GetDetour() const;

	void Enable();
	void Disable();

private:
	uintptr_t target_;
	uintptr_t detour_;
	uintptr_t trampoline_;
	size_t size_;
};

_END_WINHOOKUPP_NM_