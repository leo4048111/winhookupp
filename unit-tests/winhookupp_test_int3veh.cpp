// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include "int3veh.h"

namespace {
    BOOL WINAPI DetouredWriteConsole(
        _In_ HANDLE hConsoleOutput,
        _In_reads_(nNumberOfCharsToWrite) CONST VOID* lpBuffer,
        _In_ DWORD nNumberOfCharsToWrite,
        _Out_opt_ LPDWORD lpNumberOfCharsWritten,
        _Reserved_ LPVOID lpReserved
    ) {
        printf("Detoured WriteConsole() is called...\n");
        return false;
    }
}

TEST(winhookupp_test, winhookupp_test_int3veh)
{
    using namespace WINHOOKUPP_NM;

    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwChars;
    TCHAR buf[] = "Original WriteConsole() is called...\n";
    int len = lstrlen(buf);
    
    EXPECT_EQ(WriteConsole(hConsoleOutput, buf, len, &dwChars, nullptr), true);

    Int3Veh int3veh;
    EXPECT_EQ(int3veh.Enable(&WriteConsole, &DetouredWriteConsole), true);
    EXPECT_EQ(WriteConsole(hConsoleOutput, buf, len, &dwChars, nullptr), false);
    EXPECT_EQ(int3veh.Disable(), true);
    EXPECT_EQ(WriteConsole(hConsoleOutput, buf, len, &dwChars, nullptr), true);
}