// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "vmt.h"

#include <Windows.h>

#include "memory.h"

_START_WINHOOKUPP_NM_

bool Vmt::Enable(LPVOID target, LPVOID detour, LPVOID inst, LPVOID* origin) noexcept
{
    if (IsEnabled()) return false;

    if (inst == nullptr) return false;

    // check if the target and detour address are executable
    auto& mm = Memory::GetInstance();
    if (!mm.IsExecutableAddress(target) || !mm.IsExecutableAddress(detour))
		return false;

    target_ = target;
    detour_ = detour;

    // TODO: support multiple inheritance
    vtabel_ = *reinterpret_cast<uintptr_t**>(inst);

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
        origin_ = Memory::Read<LPVOID>((LPVOID)patchPos);

        Memory::Patch((LPVOID)patchPos, detour_);
        goto end;
    } while (true);

    // target is not a vcall thunk, directly patch vtable bytes

    disp_ = 0;
    for (;;)
    {
        uintptr_t patchPos = (uintptr_t)vtabel_ + disp_;

        if (!mm.IsExecutableAddress((LPVOID)patchPos)) return false;

        if (patchPos == (uintptr_t)target)
        {
			origin_ = Memory::Read<LPVOID>((LPVOID)patchPos);
			Memory::Patch((LPVOID)patchPos, detour_);
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

    uintptr_t patchPos = (uintptr_t)vtabel_ + disp_;
    Memory::Patch((LPVOID)patchPos, origin_);

    disp_ = 0;
    origin_ = nullptr;
    vtabel_ = nullptr;
    detour_ = nullptr;
    target_ = nullptr;

    enabled_ = false;
    return true;
}

_END_WINHOOKUPP_NM_