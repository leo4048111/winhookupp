#include <gtest/gtest.h>

#include "memory.h"

TEST(winhookupp_test, winhookupp_test_memory)
{
    using namespace WINHOOKUPP_NM;

    Memory& memory = Memory::GetInstance();

    LPVOID funcAddr = (LPVOID)(&WriteConsole);

    EXPECT_EQ(memory.IsExecutableAddress(funcAddr), true);

    LPVOID buffer = new BYTE[64];

    EXPECT_EQ(memory.IsExecutableAddress(buffer), false);
}