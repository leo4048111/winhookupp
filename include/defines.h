// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <cstdint>
#include <Windows.h>

#define WINHOOKUPP_NM winhookupp

#define _START_WINHOOKUPP_NM_ namespace winhookupp{

#define _END_WINHOOKUPP_NM_ };

#if defined(_M_X64) || defined(__x86_64__)
#include "hde/hde64.h"
typedef hde64s HDE;
#define HDE_DISASM(code, hs) hde64_disasm(code, hs)
#else
#include "hde/hde32.h"
typedef hde32s HDE;
#define HDE_DISASM(code, hs) hde32_disasm(code, hs)
#endif