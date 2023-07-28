#include "trampoline.h"

#include <Windows.h>

#include "defines.h"
#include "mem.h"


_START_HOOKUP_NM_

namespace {
}

TrampolineHook::TrampolineHook(uintptr_t target, uintptr_t detour, size_t size)
	: target_(target), detour_(detour), size_(size)
{
}

TrampolineHook::~TrampolineHook()
{
	Disable();
}

size_t TrampolineHook::GetSize() const
{
	return size_;
}

uintptr_t TrampolineHook::GetTarget() const
{
	return target_;
}

uintptr_t TrampolineHook::GetDetour() const
{
	return detour_;
}

void TrampolineHook::Enable()
{
	if (size_ < 5) return; // if size < 5, can't hook

	// Allocate a page for the trampoline.
	trampoline_ = reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

	// Copy the original bytes to the trampoline.
	memcpy_s(reinterpret_cast<void*>(trampoline_), 0x1000, reinterpret_cast<void*>(target_), 0x1000);

	// Calculate the offset from the trampoline to the target.
	uintptr_t offset = target_ - trampoline_ - 5;

	// Write the jump instruction to the trampoline.
	*reinterpret_cast<uint8_t*>(trampoline_) = 0xE9;

	// Write the offset to the trampoline.
	*reinterpret_cast<uintptr_t*>(trampoline_ + 1) = offset;

	// Calculate the offset from the detour to the target.
	offset = detour_ - target_ - 5;

	// Write the jump instruction to the target.
	DWORD oldProtect;
	VirtualProtect(reinterpret_cast<void*>(target_), 5, PAGE_EXECUTE_READWRITE, &oldProtect);
	*reinterpret_cast<uint8_t*>(target_) = 0xE9;

	// Write the offset to the target.
	*reinterpret_cast<uintptr_t*>(target_ + 1) = offset;
	VirtualProtect(reinterpret_cast<void*>(target_), 5, oldProtect, &oldProtect);
}

void TrampolineHook::Disable()
{
	if (trampoline_ == 0)
		return;

	// Calculate the offset from the trampoline to the target.
	uintptr_t offset = target_ - trampoline_ - 5;

	// Write the jump instruction to the trampoline.
	*reinterpret_cast<uint8_t*>(trampoline_) = 0xE9;

	// Write the offset to the trampoline.
	*reinterpret_cast<uintptr_t*>(trampoline_ + 1) = offset;

	// Calculate the offset from the detour to the target.
	offset = trampoline_ - detour_ - 5;

	// Write the jump instruction to the detour.
	DWORD oldProtect;
	VirtualProtect(reinterpret_cast<void*>(detour_), 5, PAGE_EXECUTE_READWRITE, &oldProtect);
	*reinterpret_cast<uint8_t*>(detour_) = 0xE9;

	// Write the offset to the detour.
	*reinterpret_cast<uintptr_t*>(detour_ + 1) = offset;
	VirtualProtect(reinterpret_cast<void*>(detour_), 5, oldProtect, &oldProtect);

	// Free the trampoline.
	VirtualFree(reinterpret_cast<void*>(trampoline_), 0, MEM_RELEASE);
	trampoline_ = 0;
}

_END_HOOKUP_NM_