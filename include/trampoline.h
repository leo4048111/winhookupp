#pragma once

#include "defines.h"
#include "hook.h"

_START_WINHOOKUPP_NM_
class Trampoline : public virtual Hook
{
private:
	bool CreateTrampolineFunction() noexcept;

public:
    virtual bool Enable(LPVOID target, LPVOID detour) noexcept override;

    virtual bool Disable() noexcept override;
	
private:
    LPVOID target_;         // [In] Address of the target function.
    LPVOID detour_;         // [In] Address of the detour function.
    LPVOID trampoline_;     // [In] Buffer address for the trampoline and relay function.

#if defined(_M_X64) || defined(__x86_64__)
    LPVOID relay_;          // [Out] Address of the relay function.
#endif
    bool   patchAbove_;      // [Out] Should use the hot patch area?
    size_t   nIP_;             // [Out] Number of the instruction boundaries.
    uint8_t  oldIPs_[8];       // [Out] Instruction boundaries of the target function.
    uint8_t  newIPs_[8];       // [Out] Instruction boundaries of the trampoline function.

    bool queueEnable_{ false };
    uint8_t backup_[8];
};

_END_WINHOOKUPP_NM_