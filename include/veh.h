#pragma once

#include "defines.h"
#include "hook.h"

_START_WINHOOKUPP_NM_
class Veh: public virtual Hook
{
public:
    virtual BOOL Enable(LPVOID target, LPVOID detour) noexcept override;

    virtual BOOL Disable() noexcept override;

private:
    DWORD old_protect_;
    PVOID hVeh_;
    LPVOID target_;         // [In] Address of the target function.
    LPVOID detour_;         // [In] Address of the detour function.
};

_END_WINHOOKUPP_NM_
