#pragma once

#include "defines.h"

class Hook
{
public:
    Hook() noexcept = default;
    virtual ~Hook() noexcept = default;

public:
    virtual bool Enable(
        _In_  LPVOID target, 
        _In_  LPVOID detour, 
        _Out_  LPVOID* origin = nullptr) noexcept = 0;

    virtual bool Disable() noexcept = 0;

    bool IsEnabled() const noexcept {
        return enabled_;
    }

protected:
    bool enabled_{ false };
};