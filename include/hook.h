#pragma once

#include "defines.h"

class Hook
{
public:
    Hook() noexcept = default;
    virtual ~Hook() noexcept = default;

private:
    virtual bool Enable(LPVOID target, LPVOID detour) noexcept = 0;

    virtual bool Disable() noexcept = 0;
};