// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "defines.h"
#include "hook.h"

_START_WINHOOKUPP_NM_

// Exception triggered by page guard violation
class Veh: public virtual Hook
{
public:

#ifdef WINHOOKUPP_EXTERNAL_USAGE
    virtual ~Veh() noexcept override {
        if (IsEnabled()) DisableEx();
    }
#else
    virtual ~Veh() noexcept override {
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
    DWORD old_protect_;
    LPVOID target_;         // [In] Address of the target function.
    LPVOID detour_;         // [In] Address of the detour function.
};

_END_WINHOOKUPP_NM_
