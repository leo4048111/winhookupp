#include "trampoline.h"
#include "defines.h"

#if defined(_M_X64) || defined(__x86_64__)
#include "hde/hde64.h"
typedef hde64s HDE;
#define HDE_DISASM(code, hs) hde64_disasm(code, hs)
#else
#include "hde/hde32.h"
typedef hde32s HDE;
#define HDE_DISASM(code, hs) hde32_disasm(code, hs)
#endif

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

    do
    {
        HDE       hs;
        UINT      copySize;
        LPVOID    copySrc;
        uintptr_t oldInst = (uintptr_t)target_ + oldPos;
        uintptr_t newInst = (uintptr_t)trampoline_ + newPos;

        copySize = HDE_DISASM((LPVOID)oldInst, &hs);
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
            PUINT32 pRelAddr;

            // Avoid using memcpy to reduce the footprint.

            Memory::Copy((LPVOID)instBuf, (LPBYTE)oldInst, copySize);

            copySrc = instBuf;

            // Relative address is stored at (instruction length - immediate value length - 4).
            pRelAddr = (uint32_t*)(instBuf + hs.len - ((hs.flags & 0x3C) >> 2) - 4);
            *pRelAddr
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
        Memory::GetInstance().Copy((LPBYTE)trampoline_ + newPos, (LPBYTE)copySrc, copySize);

        newPos += copySize;
        oldPos += hs.len;
    } while (!finished);

    // Is there enough place for a long jump?
    if (oldPos < sizeof(JmpRel)
        && !Memory::IsCodePadding((LPBYTE)target_ + oldPos, sizeof(JmpRel) - oldPos))
    {
        // Is there enough place for a short jump?
        if (oldPos < sizeof(JmpRelShort)
            && !Memory::IsCodePadding((LPBYTE)target_ + oldPos, sizeof(JmpRelShort) - oldPos))
        {
            return false;
        }

        // Can we place the long jump above the function?
        if (!Memory::IsExecutableAddress((LPBYTE)target_ - sizeof(JmpRel)))
            return false;

        if (!Memory::IsCodePadding((LPBYTE)target_ - sizeof(JmpRel), sizeof(JmpRel)))
            return false;

        patchAbove_ = true;
    }

#if defined(_M_X64) || defined(__x86_64__)
    // Create a relay function.
    jmp.address = (uintptr_t)detour_;

    relay_ = (LPBYTE)trampoline_ + newPos;
    Memory::Copy((LPVOID)relay_, (LPBYTE)& jmp, sizeof(jmp));
#endif

    return true;
}

bool Trampoline::Enable(LPVOID target, LPVOID detour) noexcept
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
    if (patchAbove_)
    {
        Memory::Copy((LPVOID)backup_, (LPBYTE)target_ - sizeof(JmpRel), sizeof(JmpRel) + sizeof(JmpRelShort));
    }
    else
    {
        Memory::Copy((LPVOID)backup_, (LPBYTE)target_, sizeof(JmpRel));
    }

	return true;
}

bool Trampoline::Disable() noexcept
{
	return true;
}

_END_WINHOOKUPP_NM_