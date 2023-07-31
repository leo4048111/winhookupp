#pragma once

#include "defines.h"

class Hook
{
public:
    Hook() noexcept = default;
    virtual ~Hook() noexcept = default;

public:
    virtual bool Enable(LPVOID target, LPVOID detour) noexcept = 0;

    virtual bool Disable() noexcept = 0;

    bool IsEnabled() const noexcept {
        return enabled_;
    }

protected:
    bool enabled_{ false };
};