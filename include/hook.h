// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "defines.h"

class Hook
{
public:
    Hook() noexcept = default;
    virtual ~Hook() noexcept = default;

public:
#ifdef WINHOOKUPP_EXTERNAL_USAGE
    virtual bool EnableEx(
        _In_ HANDLE hProcess, // handle to target process
        _In_ LPVOID target,   // target function
        _In_ LPVOID detour,   // detour function, will be called instead of target after enabling hook. This value should be the address of the detour function in the remote process.
                              // Make sure you have correctly injected the detour function into remote process, otherwise calling EnableEx would very likely crash the remote process.
        _Out_ LPVOID* origin = nullptr) noexcept = 0;

    virtual bool DisableEx() noexcept = 0;
#else
    virtual bool Enable(
        _In_ LPVOID target,   // target function
        _In_ LPVOID detour,   // detour function, will be called instead of target after enabling hook
        _Out_ LPVOID *origin = nullptr) noexcept = 0;

    virtual bool Disable() noexcept = 0;
#endif
    bool IsEnabled() const noexcept
    {
        return enabled_;
    }

protected:
    bool enabled_{false};

#ifdef WINHOOKUPP_EXTERNAL_USAGE
	HANDLE hProcess_{nullptr};
#endif
};