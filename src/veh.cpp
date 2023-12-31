// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "veh.h"

#include <Windows.h>
#include <unordered_map>
#include <mutex>

#include "memory.h"

_START_WINHOOKUPP_NM_

namespace
{
    ::std::mutex g_mutex;
    ::std::unordered_map<uintptr_t, uintptr_t> g_active_veh_hooks;
    LPVOID g_hVeh = nullptr;
    bool g_veh_hook_installed = false;

    LONG WINAPI VehHandler(PEXCEPTION_POINTERS exception_pointers)
    {
        uintptr_t ip;
#if defined(_M_X64) || defined(__x86_64__)
        ip = exception_pointers->ContextRecord->Rip;
#else 
        ip = exception_pointers->ContextRecord->Eip;
#endif
        switch (exception_pointers->ExceptionRecord->ExceptionCode)
        {
        case STATUS_GUARD_PAGE_VIOLATION:
        {
            ::std::lock_guard<::std::mutex> lock(g_mutex);
            if (g_active_veh_hooks.find(ip) != g_active_veh_hooks.end())
            {
#if defined(_M_X64) || defined(__x86_64__)
                exception_pointers->ContextRecord->Rip = g_active_veh_hooks[ip];
#else
                exception_pointers->ContextRecord->Eip = g_active_veh_hooks[ip];
#endif
            }
            exception_pointers->ContextRecord->EFlags |= 0x100; // Will trigger an STATUS_SINGLE_STEP exception right after the next instruction get executed. In short, we come right back into this exception handler 1 instruction later
            return EXCEPTION_CONTINUE_EXECUTION;                // Continue to next instruction
        }
        case STATUS_SINGLE_STEP:
        {
            DWORD dwOldProtect;
            for (auto &hook : g_active_veh_hooks)
                VirtualProtect((LPVOID)hook.first, 1, PAGE_EXECUTE_READ | PAGE_GUARD, &dwOldProtect);

            return EXCEPTION_CONTINUE_EXECUTION; // Continue the next instruction
        }
        default:
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }
}

#ifdef WINHOOKUPP_EXTERNAL_USAGE
bool Veh::EnableEx(HANDLE hProcess, LPVOID target, LPVOID detour, LPVOID* origin) noexcept
{
    return false;
}

bool Veh::DisableEx() noexcept
{
    return false;
}
#else
bool Veh::Enable(LPVOID target, LPVOID detour, LPVOID* origin) noexcept
{
    // check if the target and detour address are executable
    auto& mm = Memory::GetInstance();
    if (!mm.IsExecutableAddress(target) || !mm.IsExecutableAddress(detour))
        return false;

    target_ = target;
    detour_ = detour;

    {
		::std::lock_guard<::std::mutex> lock(g_mutex);
        if (g_active_veh_hooks.find((uintptr_t)target_) != g_active_veh_hooks.end())
            return false;

        g_active_veh_hooks.insert({(uintptr_t)target_, (uintptr_t)detour_});
    }

    if (!g_veh_hook_installed)
    {
        g_hVeh = AddVectoredExceptionHandler(true, VehHandler);
        g_veh_hook_installed = true;
    }

    if (!VirtualProtect((LPVOID)target_, 1, PAGE_EXECUTE_READ | PAGE_GUARD, &old_protect_))
        return false;
    
    // TODO: implementation
    if (origin) *origin = nullptr;
    enabled_ = true;
    return true;
}

bool Veh::Disable() noexcept
{
    bool should_remove_veh = false;
    {
        ::std::lock_guard<::std::mutex> lock(g_mutex);
        if (g_active_veh_hooks.find((uintptr_t)target_) == g_active_veh_hooks.end())
            return false;

        g_active_veh_hooks.erase((uintptr_t)target_);
        should_remove_veh = g_active_veh_hooks.empty();
    }

    if (should_remove_veh)
    {
        if (!RemoveVectoredExceptionHandler(g_hVeh))
            return false;

        g_veh_hook_installed = false;
    }

    DWORD foo;
    if (!VirtualProtect((LPVOID)target_, 1, old_protect_, &foo))
        return false;

    enabled_ = false;
    return true;
}
#endif

_END_WINHOOKUPP_NM_