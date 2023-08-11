// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include "veh.h"

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

#ifdef WINHOOKUPP_EXTERNAL_USAGE

#else
TEST(winhookupp_test, winhookupp_test_veh)
{
    using namespace WINHOOKUPP_NM;

    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwChars;
    TCHAR buf[] = "Original WriteConsole() is called...\n";
    int len = lstrlen(buf);
    
    EXPECT_EQ(WriteConsole(hConsoleOutput, buf, len, &dwChars, nullptr), true);

    Veh veh;
    EXPECT_EQ(veh.Enable(&WriteConsole, &DetouredWriteConsole), true);
    EXPECT_EQ(WriteConsole(hConsoleOutput, buf, len, &dwChars, nullptr), false);
    EXPECT_EQ(veh.Disable(), true);
    EXPECT_EQ(WriteConsole(hConsoleOutput, buf, len, &dwChars, nullptr), true);
}
#endif