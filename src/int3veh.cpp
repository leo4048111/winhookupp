// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "int3veh.h"

#include <Windows.h>
#include <unordered_map>
#include <mutex>

#include "memory.h"
#include "instructions.h"

_START_WINHOOKUPP_NM_

namespace
{
    ::std::mutex g_mutex;
    ::std::unordered_map<uintptr_t, uintptr_t> g_active_veh_hooks;
    ::std::unordered_map<uintptr_t, BYTE> g_patched_bytes;
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
        case EXCEPTION_BREAKPOINT:
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
            return EXCEPTION_CONTINUE_EXECUTION; // Continue the next instruction
        }
        default:
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }
}

bool Int3Veh::Enable(LPVOID target, LPVOID detour, LPVOID inst, LPVOID* origin) noexcept
{
    if (IsEnabled()) return false;

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

        g_active_veh_hooks.insert({ (uintptr_t)target_, (uintptr_t)detour_ });
        targetPatchPos_ = target_;
        BYTE patched = *(BYTE*)targetPatchPos_;
        g_patched_bytes.insert({ (uintptr_t)target_, patched });
        Memory::Patch(targetPatchPos_, Int3());
    }

    if (!g_veh_hook_installed)
    {
        g_hVeh = AddVectoredExceptionHandler(true, VehHandler);
        g_veh_hook_installed = true;
    }


    // TODO: implementation
    if (origin) *origin = nullptr;
    enabled_ = true;
    return true;
}

bool Int3Veh::Disable() noexcept
{
    if (!IsEnabled()) return false;

    bool should_remove_veh = false;
    {
        ::std::lock_guard<::std::mutex> lock(g_mutex);
        if (g_active_veh_hooks.find((uintptr_t)target_) == g_active_veh_hooks.end())
            return false;

        g_active_veh_hooks.erase((uintptr_t)target_);
        should_remove_veh = g_active_veh_hooks.empty();

        BYTE patched = g_patched_bytes[(uintptr_t)target_];
        Memory::Patch(targetPatchPos_, patched);
    }

    if (should_remove_veh)
    {
        if (!RemoveVectoredExceptionHandler(g_hVeh))
            return false;

        g_hVeh = nullptr;
        g_veh_hook_installed = false;
    }

    enabled_ = false;
    target_ = nullptr;
    detour_ = nullptr;
    targetPatchPos_ = nullptr;
    return true;
}

_END_WINHOOKUPP_NM_