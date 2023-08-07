// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "defines.h"
#include "hook.h"

_START_WINHOOKUPP_NM_

// Exception triggered by int3
class Vmt: public virtual Hook
{
public:
    virtual ~Vmt() noexcept override {
        if (IsEnabled()) Disable();
    }

    virtual bool Enable(LPVOID target, LPVOID detour, LPVOID inst, LPVOID* origin = nullptr) noexcept override;

    virtual bool Disable() noexcept override;

private:
    LPVOID target_;         // [In] Address of the target function.
    LPVOID detour_;         // [In] Address of the detour function.

    uintptr_t* vtabel_;
    uint8_t disp_;
    LPVOID origin_;

};

_END_WINHOOKUPP_NM_