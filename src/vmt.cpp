// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "vmt.h"

#include <Windows.h>

#include "memory.h"

_START_WINHOOKUPP_NM_
#ifdef WINHOOKUPP_EXTERNAL_USAGE
bool Vmt::EnableEx(HANDLE hProcess, LPVOID target, LPVOID detour, LPVOID inst, LPVOID* origin) noexcept
{
    if (inst == nullptr) return false;
    inst_ = inst;
    hProcess_ = hProcess;
    return EnableEx(hProcess, target, detour, origin);
}

bool Vmt::EnableEx(HANDLE hProcess, LPVOID target, LPVOID detour, LPVOID* origin) noexcept
{
    if (IsEnabled()) return false;

    // check if the target and detour address are executable
    auto& mm = Memory::GetInstance(hProcess);
    if (!mm.IsExecutableAddress(target) || !mm.IsExecutableAddress(detour))
        return false;

    target_ = target;
    detour_ = detour;

    // TODO: support multiple inheritance
    vtabel_ = mm.ReadEx<uintptr_t*>(inst_);

    // check if target is a vcall thunk
    do {
        HDE hs;
        size_t copySize;
        LPBYTE inst = (LPBYTE)(target);

        const size_t instBufferSize = 32;
        LPBYTE instBuffer = (LPBYTE)alloca(instBufferSize);
        mm.CopyEx(instBuffer, (LPBYTE)inst, instBufferSize);

        copySize = HDE_DISASM(instBuffer, &hs);
        if (hs.flags & F_ERROR)
            return false;

        // first inst should be a mov rax/eax, DWORD PTR [RCX/ECX]
        // which gets the address of the vtable
        if (!(hs.opcode == 0x8B && hs.modrm == 0x01)) break;

        // second inst should be a jump to virtual method
        instBuffer += copySize;
        copySize = HDE_DISASM(instBuffer, &hs);
        if (hs.flags & F_ERROR)
            return false;

        // jmp QWORD/DWORD PTR [rax/eax + offset]
        if (!(hs.opcode == 0xFF))
            break;

        // get displacement
        disp_ = (copySize == 2) ? 0x00 : hs.disp.disp8;

        uintptr_t patchPos = (uintptr_t)vtabel_ + disp_;
        origin_ = mm.ReadEx<LPVOID>((LPVOID)patchPos);

        mm.PatchEx((LPVOID)patchPos, detour_);
        goto end;
    } while (true);

    // target is not a vcall thunk, directly patch vtable bytes
    disp_ = 0;
    for (;;)
    {
        uintptr_t patchPos = (uintptr_t)vtabel_ + disp_;
        uintptr_t funcAddr = mm.ReadEx<uintptr_t>((LPVOID)patchPos);

        if (!mm.IsExecutableAddress((LPVOID)funcAddr))return false;

        if (funcAddr == (uintptr_t)target)
        {
            origin_ = (LPVOID)funcAddr;
            mm.PatchEx((LPVOID)patchPos, detour_);
            goto end;
        }

        disp_ += sizeof(uintptr_t);
    }

end:
    if (origin) *origin = origin_;
    enabled_ = true;
    return true;
}

bool Vmt::DisableEx() noexcept
{
    if (!IsEnabled()) return false;

    auto& mm = Memory::GetInstance(hProcess_);

    uintptr_t patchPos = (uintptr_t)vtabel_ + disp_;
    mm.PatchEx((LPVOID)patchPos, origin_);

    disp_ = 0;
    origin_ = nullptr;
    vtabel_ = nullptr;
    detour_ = nullptr;
    target_ = nullptr;

    enabled_ = false;
    return true;
}
#else
bool Vmt::Enable(LPVOID target, LPVOID detour, LPVOID inst, LPVOID* origin) noexcept
{
    if (inst == nullptr) return false;
    inst_ = inst;
    return Enable(target, detour, origin);
}

bool Vmt::Enable(LPVOID target, LPVOID detour, LPVOID* origin) noexcept
{
    if (IsEnabled()) return false;

    // check if the target and detour address are executable
    auto& mm = Memory::GetInstance();
    if (!mm.IsExecutableAddress(target) || !mm.IsExecutableAddress(detour))
		return false;

    target_ = target;
    detour_ = detour;

    // TODO: support multiple inheritance
    vtabel_ = *reinterpret_cast<uintptr_t**>(inst_);

    // check if target is a vcall thunk
    do {
        HDE hs;
        size_t copySize;
        LPBYTE inst = (LPBYTE)(target);

        copySize = HDE_DISASM(inst, &hs);
        if (hs.flags & F_ERROR)
            return false;
           
        // first inst should be a mov rax/eax, DWORD PTR [RCX/ECX]
        // which gets the address of the vtable
        if (!(hs.opcode == 0x8B && hs.modrm == 0x01)) break;

        // second inst should be a jump to virtual method
        inst += copySize;
        copySize = HDE_DISASM(inst, &hs);
        if (hs.flags & F_ERROR)
            return false;

        // jmp QWORD/DWORD PTR [rax/eax + offset]
        if(!(hs.opcode == 0xFF))
            break;

        // get displacement
        disp_ = (copySize == 2) ? 0x00 : hs.disp.disp8;

        uintptr_t patchPos = (uintptr_t)vtabel_ + disp_;
        origin_ = mm.Read<LPVOID>((LPVOID)patchPos);

        mm.Patch((LPVOID)patchPos, detour_);
        goto end;
    } while (true);

    // target is not a vcall thunk, directly patch vtable bytes
    disp_ = 0;
    for (;;)
    {
        uintptr_t patchPos = (uintptr_t)vtabel_ + disp_;
        uintptr_t funcAddr = mm.Read<uintptr_t>((LPVOID)patchPos);

        if (!mm.IsExecutableAddress((LPVOID)funcAddr))return false;

        if (funcAddr == (uintptr_t)target)
        {
            origin_ = (LPVOID)funcAddr;
            mm.Patch((LPVOID)patchPos, detour_);
            goto end;
        }

        disp_ += sizeof(uintptr_t);
    }

end:
    if(origin) *origin = origin_;
    enabled_ = true;
    return true;
}

bool Vmt::Disable() noexcept
{
    if(!IsEnabled()) return false;

    auto& mm = Memory::GetInstance();

    uintptr_t patchPos = (uintptr_t)vtabel_ + disp_;
    mm.Patch((LPVOID)patchPos, origin_);

    disp_ = 0;
    origin_ = nullptr;
    vtabel_ = nullptr;
    detour_ = nullptr;
    target_ = nullptr;

    enabled_ = false;
    return true;
}
#endif
_END_WINHOOKUPP_NM_