// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "defines.h"

_START_WINHOOKUPP_NM_

#pragma pack(push, 1)
// Structs for writing x86/x64 instructions.

// 8-bit relative jump.
typedef struct JmpRelShort
{
    uint8_t opcode; // EB xx: JMP +2+xx
    uint8_t operand;
} JmpRelShort;

// 32-bit direct relative jump/call.
typedef struct JmpRel
{
    uint8_t opcode;   // E9/E8 xxxxxxxx: JMP/CALL +5+xxxxxxxx
    uint32_t operand; // Relative destination address
} JmpRel, CallRel;

// 64-bit indirect absolute jump.
typedef struct JmpAbs
{
    uint8_t opcode0; // FF25 00000000: JMP [+6]
    uint8_t opcode1;
    uint32_t dummy;
    uint64_t address; // Absolute destination address
} JmpAbs;

// 64-bit indirect absolute call.
typedef struct CallAbs
{
    uint8_t opcode0; // FF15 00000002: CALL [+6]
    uint8_t opcode1;
    uint32_t dummy0;
    uint8_t dummy1; // EB 08:         JMP +10
    uint8_t dummy2;
    uint64_t address; // Absolute destination address
} CallAbs;

// 32-bit direct relative conditional jumps.
typedef struct JccRel
{
    uint8_t opcode0; // 0F8* xxxxxxxx: J** +6+xxxxxxxx
    uint8_t opcode1;
    uint32_t operand; // Relative destination address
} JccRel;

// 64bit indirect absolute conditional jumps that x64 lacks.
typedef struct JccAbs
{
    uint8_t opcode; // 7* 0E:         J** +16
    uint8_t dummy0;
    uint8_t dummy1; // FF25 00000000: JMP [+6]
    uint8_t dummy2;
    uint32_t dummy3;
    uint64_t address; // Absolute destination address
} JccAbs;

// int 3 breakpoint.
typedef struct Int3
{
    uint8_t opcode{0xCC}; // CC:            INT 3
} Int3;

#pragma pack(pop)

_END_WINHOOKUPP_NM_