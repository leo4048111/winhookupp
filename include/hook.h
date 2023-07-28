#pragma once

#include "defines.h"

class Hook
{
public:
    Hook() noexcept = default;
    virtual ~Hook() noexcept = default;

private:
    virtual BOOL Enable(LPVOID target, LPVOID detour) noexcept = 0;

    virtual BOOL Disable() noexcept = 0;
};