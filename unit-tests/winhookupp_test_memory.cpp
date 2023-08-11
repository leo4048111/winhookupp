// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include "memory.h"

#ifdef WINHOOKUPP_EXTERNAL_USAGE

#else
TEST(winhookupp_test, winhookupp_test_memory)
{
    using namespace WINHOOKUPP_NM;

    Memory& memory = Memory::GetInstance();

    LPVOID funcAddr = (LPVOID)(&WriteConsole);

    EXPECT_EQ(memory.IsExecutableAddress(funcAddr), true);

    LPVOID buffer = new BYTE[64];

    EXPECT_EQ(memory.IsExecutableAddress(buffer), false);

    LPVOID alloc = memory.AllocateBuffer(funcAddr);

    EXPECT_EQ(memory.IsExecutableAddress(alloc), true);

    memory.FreeBuffer(alloc);
}
#endif