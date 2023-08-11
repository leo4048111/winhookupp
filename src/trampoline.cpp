// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "trampoline.h"
#include "defines.h"

#include <Windows.h>

#include "memory.h"
#include "instructions.h"

// Maximum size of a trampoline function.
#if defined(_M_X64) || defined(__x86_64__)
#define TRAMPOLINE_MAX_SIZE (MEMORY_SLOT_SIZE - sizeof(JmpAbs))
#else
#define TRAMPOLINE_MAX_SIZE MEMORY_SLOT_SIZE
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(A) (sizeof(A)/sizeof((A)[0]))
#endif

_START_WINHOOKUPP_NM_

bool Trampoline::CreateTrampolineFunction() noexcept
{
#if defined(_M_X64) || defined(__x86_64__)
    CallAbs call = {
        0xFF, 0x15, 0x00000002, // FF15 00000002: CALL [RIP+8]
        0xEB, 0x08,             // EB 08:         JMP +10
        0x0000000000000000ULL   // Absolute destination address
    };
    JmpAbs jmp = {
        0xFF, 0x25, 0x00000000, // FF25 00000000: JMP [RIP+6]
        0x0000000000000000ULL   // Absolute destination address
    };
    JccAbs jcc = {
        0x70, 0x0E,             // 7* 0E:         J** +16
        0xFF, 0x25, 0x00000000, // FF25 00000000: JMP [RIP+6]
        0x0000000000000000ULL   // Absolute destination address
    };
#else
    CallRel call = {
        0xE8,                   // E8 xxxxxxxx: CALL +5+xxxxxxxx
        0x00000000              // Relative destination address
    };
    JmpRel jmp = {
        0xE9,                   // E9 xxxxxxxx: JMP +5+xxxxxxxx
        0x00000000              // Relative destination address
    };
    JccRel jcc = {
        0x0F, 0x80,             // 0F8* xxxxxxxx: J** +6+xxxxxxxx
        0x00000000              // Relative destination address
    };
#endif

    UINT8     oldPos = 0;
    UINT8     newPos = 0;
    uintptr_t jmpDest = 0;     // Destination address of an internal jump.
    bool      finished = false; // Is the function completed?
#if defined(_M_X64) || defined(__x86_64__)
    UINT8     instBuf[16];
#endif

    patchAbove_ = false;
    nIP_ = 0;

#ifdef WINHOOKUPP_EXTERNAL_USAGE
    auto& mm = Memory::GetInstance(hProcess_);
#else
    auto& mm = Memory::GetInstance();
#endif

    do
    {
        HDE hs;
        size_t copySize;
        LPVOID copySrc;
        uintptr_t oldInst = (uintptr_t)target_ + oldPos;
        uintptr_t newInst = (uintptr_t)trampoline_ + newPos;
        const size_t copyInstSize = 16;
        LPBYTE oldInstBuffer = (LPBYTE)alloca(copyInstSize);

#ifdef WINHOOKUPP_EXTERNAL_USAGE
        mm.ReadEx(oldInstBuffer, (LPBYTE)oldInst, copyInstSize);
#else
        mm.Read(oldInstBuffer, (LPBYTE)oldInst, copyInstSize);
#endif

        copySize = HDE_DISASM((LPVOID)oldInstBuffer, &hs);
        if (hs.flags & F_ERROR)
            return false;

        copySrc = (LPVOID)oldInst;
        if (oldPos >= sizeof(JmpRel))
        {
            // The trampoline function is long enough.
            // Complete the function with the jump to the target function.
#if defined(_M_X64) || defined(__x86_64__)
            jmp.address = oldInst;
#else
            jmp.operand = (UINT32)(oldInst - (newInst + sizeof(jmp)));
#endif
            copySrc = &jmp;
            copySize = sizeof(jmp);

            finished = true;
        }
#if defined(_M_X64) || defined(__x86_64__)
        else if ((hs.modrm & 0xC7) == 0x05)
        {
            // Instructions using RIP relative addressing. (ModR/M = 00???101B)

            // Modify the RIP relative address.
            uint32_t* relAddr;

            // Avoid using memcpy to reduce the footprint.

#ifdef WINHOOKUPP_EXTERNAL_USAGE
            mm.CopyEx((LPVOID)instBuf, (LPBYTE)oldInst, copySize);
#else
            mm.Copy((LPVOID)instBuf, (LPBYTE)oldInst, copySize);
#endif

            copySrc = instBuf;

            // Relative address is stored at (instruction length - immediate value length - 4).
            relAddr = (uint32_t*)(instBuf + hs.len - ((hs.flags & 0x3C) >> 2) - 4);
            *relAddr
                = (uint32_t)((oldInst + hs.len + (INT32)hs.disp.disp32) - (newInst + hs.len));

            // Complete the function if JMP (FF /4).
            if (hs.opcode == 0xFF && hs.modrm_reg == 4)
                finished = true;
        }
#endif
        else if (hs.opcode == 0xE8)
        {
            // Direct relative CALL
            uintptr_t dest = oldInst + hs.len + (INT32)hs.imm.imm32;
#if defined(_M_X64) || defined(__x86_64__)
            call.address = dest;
#else
            call.operand = (UINT32)(dest - (newInst + sizeof(call)));
#endif
            copySrc = &call;
            copySize = sizeof(call);
        }
        else if ((hs.opcode & 0xFD) == 0xE9)
        {
            // Direct relative JMP (EB or E9)
            uintptr_t dest = oldInst + hs.len;

            if (hs.opcode == 0xEB) // isShort jmp
                dest += (INT8)hs.imm.imm8;
            else
                dest += (INT32)hs.imm.imm32;

            // Simply copy an internal jump.
            if ((uintptr_t)target_ <= dest
                && dest < ((uintptr_t)target_ + sizeof(JmpRel)))
            {
                if (jmpDest < dest)
                    jmpDest = dest;
            }
            else
            {
#if defined(_M_X64) || defined(__x86_64__)
                jmp.address = dest;
#else
                jmp.operand = (UINT32)(dest - (newInst + sizeof(jmp)));
#endif
                copySrc = &jmp;
                copySize = sizeof(jmp);

                // Exit the function if it is not in the branch.
                finished = (oldInst >= jmpDest);
            }
        }
        else if ((hs.opcode & 0xF0) == 0x70
            || (hs.opcode & 0xFC) == 0xE0
            || (hs.opcode2 & 0xF0) == 0x80)
        {
            // Direct relative Jcc
            uintptr_t dest = oldInst + hs.len;

            if ((hs.opcode & 0xF0) == 0x70      // Jcc
                || (hs.opcode & 0xFC) == 0xE0)  // LOOPNZ/LOOPZ/LOOP/JECXZ
                dest += (INT8)hs.imm.imm8;
            else
                dest += (INT32)hs.imm.imm32;

            // Simply copy an internal jump.
            if ((uintptr_t)target_ <= dest
                && dest < ((uintptr_t)target_ + sizeof(JmpRel)))
            {
                if (jmpDest < dest)
                    jmpDest = dest;
            }
            else if ((hs.opcode & 0xFC) == 0xE0)
            {
                // LOOPNZ/LOOPZ/LOOP/JCXZ/JECXZ to the outside are not supported.
                return false;
            }
            else
            {
                UINT8 cond = ((hs.opcode != 0x0F ? hs.opcode : hs.opcode2) & 0x0F);
#if defined(_M_X64) || defined(__x86_64__)
                // Invert the condition in x64 mode to simplify the conditional jump logic.
                jcc.opcode = 0x71 ^ cond;
                jcc.address = dest;
#else
                jcc.opcode1 = 0x80 | cond;
                jcc.operand = (UINT32)(dest - (newInst + sizeof(jcc)));
#endif
                copySrc = &jcc;
                copySize = sizeof(jcc);
            }
        }
        else if ((hs.opcode & 0xFE) == 0xC2)
        {
            // RET (C2 or C3)

            // Complete the function if not in a branch.
            finished = (oldInst >= jmpDest);
        }

        // Can't alter the instruction length in a branch.
        if (oldInst < jmpDest && copySize != hs.len)
            return false;

        // Trampoline function is too large.
        if ((newPos + copySize) > TRAMPOLINE_MAX_SIZE)
            return false;

        // Trampoline function has too many instructions.
        if (nIP_ >= ARRAYSIZE(oldIPs_))
            return false;

        oldIPs_[nIP_] = oldPos;
        newIPs_[nIP_] = newPos;
        nIP_++;

        // Copy the instruction.
#ifdef WINHOOKUPP_EXTERNAL_USAGE
        mm.CopyEx((LPBYTE)trampoline_ + newPos, (LPBYTE)copySrc, copySize);
#else
        mm.Copy((LPBYTE)trampoline_ + newPos, (LPBYTE)copySrc, copySize);
#endif

        newPos += copySize;
        oldPos += hs.len;
    } while (!finished);

    // Is there enough place for a long jump?
    if (oldPos < sizeof(JmpRel)
        && !mm.IsCodePadding((LPBYTE)target_ + oldPos, sizeof(JmpRel) - oldPos))
    {
        // Is there enough place for a short jump?
        if (oldPos < sizeof(JmpRelShort)
            && !mm.IsCodePadding((LPBYTE)target_ + oldPos, sizeof(JmpRelShort) - oldPos))
        {
            return false;
        }

        // Can we place the long jump above the function?
        if (!mm.IsExecutableAddress((LPBYTE)target_ - sizeof(JmpRel)))
            return false;

        if (!mm.IsCodePadding((LPBYTE)target_ - sizeof(JmpRel), sizeof(JmpRel)))
            return false;

        patchAbove_ = true;
    }

#if defined(_M_X64) || defined(__x86_64__)
    // Create a relay function.
    jmp.address = (uintptr_t)detour_;

    relay_ = (LPBYTE)trampoline_ + newPos;

#ifdef WINHOOKUPP_EXTERNAL_USAGE
    mm.CopyEx((LPVOID)relay_, (LPBYTE)&jmp, sizeof(jmp));
#else
    mm.Copy((LPVOID)relay_, (LPBYTE)& jmp, sizeof(jmp));
#endif

#endif

    return true;
}

#ifdef WINHOOKUPP_EXTERNAL_USAGE
bool Trampoline::EnableEx(HANDLE hProcess, LPVOID target, LPVOID detour, LPVOID* origin) noexcept
{
    hProcess_ = hProcess;

    // check if the target and detour address are executable
    auto& mm = Memory::GetInstance(hProcess_);
    if (!mm.IsExecutableAddress(target) || !mm.IsExecutableAddress(detour))
        return false;

    trampoline_ = mm.AllocateBuffer(target);
    if (!trampoline_) return false;

    target_ = target;
    detour_ = detour;
    if (!CreateTrampolineFunction()) return false;

    enabled_ = false;
    queueEnable_ = false;

    patchedPos_ = (LPBYTE)target_;

    // check if trampoline is reachable within a 32-bit rel jmp(very likely reachable)
    // x64 needs relaying
#if defined(_M_X64) || defined(__x86_64__)
    int64_t jmpDst = (int64_t)((LPBYTE)relay_ - ((LPBYTE)patchedPos_ + sizeof(JmpRel)));
#else
    int64_t jmpDst = (int64_t)((LPBYTE)detour_ - ((LPBYTE)patchedPos_ + sizeof(JmpRel)));
#endif
    patchedSize_ = sizeof(JmpRel);

    if (patchAbove_)
    {
        (LPBYTE)patchedPos_ -= sizeof(JmpRel);
        patchedSize_ += sizeof(JmpRelShort);
    }

    // backup patched bytes
    mm.CopyEx((LPVOID)backup_, (LPBYTE)patchedPos_, patchedSize_);

    JmpRel* pJmp = (JmpRel*)patchedPos_;
    mm.PatchEx(&pJmp->opcode, (BYTE)0xE9);
    mm.PatchEx(&pJmp->operand, (uint32_t)jmpDst);

    if (patchAbove_)
    {
        JmpRelShort* pShortJmp = (JmpRelShort*)target_;
        uint8_t shortJmpDst = (UINT8)(0 - (sizeof(JmpRelShort) + sizeof(JmpRel)));
        mm.PatchEx(&pShortJmp->opcode, (char)0xEB);
        mm.PatchEx(&pShortJmp->operand, shortJmpDst);
    }

    if (origin) *origin = trampoline_;

    enabled_ = true;
    return true;
}

bool Trampoline::DisableEx() noexcept
{
    auto& mm = Memory::GetInstance(hProcess_);
    mm.PatchEx((LPVOID)patchedPos_, (LPBYTE)backup_, patchedSize_);
    enabled_ = false;
    return true;
}
#else
bool Trampoline::Enable(LPVOID target, LPVOID detour, LPVOID* origin) noexcept
{
    // check if the target and detour address are executable
    auto& mm = Memory::GetInstance();
    if (!mm.IsExecutableAddress(target) || !mm.IsExecutableAddress(detour))
        return false;

    trampoline_ = mm.AllocateBuffer(target);
    if (!trampoline_) return false;

    target_ = target;
    detour_ = detour;
    if (!CreateTrampolineFunction()) return false;

    enabled_ = false;
    queueEnable_ = false;

    patchedPos_ = (LPBYTE)target_;

    // check if trampoline is reachable within a 32-bit rel jmp(very likely reachable)
    // x64 needs relaying
#if defined(_M_X64) || defined(__x86_64__)
    int64_t jmpDst = (int64_t)((LPBYTE)relay_ - ((LPBYTE)patchedPos_ + sizeof(JmpRel)));
#else
    int64_t jmpDst = (int64_t)((LPBYTE)detour_ - ((LPBYTE)patchedPos_ + sizeof(JmpRel)));
#endif
    patchedSize_ = sizeof(JmpRel);

    if (patchAbove_)
    {
        (LPBYTE)patchedPos_ -= sizeof(JmpRel);
        patchedSize_ += sizeof(JmpRelShort);
    }

    // backup patched bytes
    mm.Copy((LPVOID)backup_, (LPBYTE)patchedPos_, patchedSize_);

    JmpRel* pJmp = (JmpRel*)patchedPos_;
    mm.Patch(&pJmp->opcode, (BYTE)0xE9);
    mm.Patch(&pJmp->operand, (uint32_t)jmpDst);

    if (patchAbove_)
    {
        JmpRelShort* pShortJmp = (JmpRelShort*)target_;
        uint8_t shortJmpDst = (UINT8)(0 - (sizeof(JmpRelShort) + sizeof(JmpRel)));
        mm.Patch(&pShortJmp->opcode, (char)0xEB);
        mm.Patch(&pShortJmp->operand, shortJmpDst);
    }

    if (origin) *origin = trampoline_;

    enabled_ = true;
    return true;
}

bool Trampoline::Disable() noexcept
{
    auto& mm = Memory::GetInstance();
    mm.Patch((LPVOID)patchedPos_, (LPBYTE)backup_, patchedSize_);
    enabled_ = false;
    return true;
}
#endif

_END_WINHOOKUPP_NM_