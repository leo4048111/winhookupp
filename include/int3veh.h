// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "defines.h"
#include "hook.h"

_START_WINHOOKUPP_NM_

// Exception triggered by int3
class Int3Veh: public virtual Hook
{
public:
#ifdef WINHOOKUPP_EXTERNAL_USAGE
    virtual ~Int3Veh() noexcept override {
        if (IsEnabled()) DisableEx();
    }
#else
    virtual ~Int3Veh() noexcept override {
        if (IsEnabled()) Disable();
    }
#endif

#ifdef WINHOOKUPP_EXTERNAL_USAGE
    virtual bool EnableEx(HANDLE hProcess, LPVOID target, LPVOID detour, LPVOID* origin = nullptr) noexcept override;

    virtual bool DisableEx() noexcept override;
#else
    virtual bool Enable(LPVOID target, LPVOID detour, LPVOID* origin = nullptr) noexcept override;

    virtual bool Disable() noexcept override;
#endif

private:
    LPVOID target_;         // [In] Address of the target function.
    LPVOID detour_;         // [In] Address of the detour function.

    LPVOID targetPatchPos_; 
};

_END_WINHOOKUPP_NM_