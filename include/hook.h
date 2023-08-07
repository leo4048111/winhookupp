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
    virtual bool Enable(
        _In_ LPVOID target,   // target function
        _In_ LPVOID detour,   // detour function, will be called instead of target after enabling hook
        _Out_ LPVOID *origin = nullptr) noexcept = 0;

    virtual bool Disable() noexcept = 0;

    bool IsEnabled() const noexcept
    {
        return enabled_;
    }

protected:
    bool enabled_{false};
};