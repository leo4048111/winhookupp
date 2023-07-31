#include <gtest/gtest.h>

#include "trampoline.h"

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

TEST(winhookupp_test, winhookupp_test_trampoline)
{
    using namespace WINHOOKUPP_NM;

    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwChars;
    TCHAR buf[] = "Original WriteConsole() is called...\n";
    int len = lstrlen(buf);
    
    EXPECT_EQ(WriteConsole(hConsoleOutput, buf, len, &dwChars, nullptr), true);

    Trampoline tramp;

    EXPECT_EQ(tramp.Enable(&WriteConsole, &DetouredWriteConsole), true);
    EXPECT_EQ(WriteConsole(hConsoleOutput, buf, len, &dwChars, nullptr), false);
    EXPECT_EQ(tramp.Disable(), true);
    EXPECT_EQ(WriteConsole(hConsoleOutput, buf, len, &dwChars, nullptr), true);
}