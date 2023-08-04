// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <windows.h>
#include <iostream>

#include "veh.h"
#include "trampoline.h"

// implementation of your detour function
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

int main(int argc, char** argv) {
    using namespace WINHOOKUPP_NM;

    // veh hooking
    Veh veh;
    veh.Enable(&WriteConsole, &DetouredWriteConsole);

    // calling detoured function
    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwChars;
    TCHAR buf[] = "Original WriteConsole() is called...\n";
    int len = lstrlen(buf);
    WriteConsole(hConsoleOutput, buf, len, &dwChars, nullptr);

    // calling original function
    // not possible in veh hooking

    // restore hook
    veh.Disable();

    // trampoline hooking
    Trampoline trampoline;
    using WriteConsole_t = decltype(WriteConsole);
    LPVOID origin;
    trampoline.Enable(&WriteConsole, &DetouredWriteConsole, &origin);

    // calling detoured function
    WriteConsole(hConsoleOutput, buf, len, &dwChars, nullptr);

    // calling original function
    WriteConsole_t* pOrigin = reinterpret_cast<WriteConsole_t*>(origin);
    pOrigin(hConsoleOutput, buf, len, &dwChars, nullptr);

    // restore hook
    trampoline.Disable();
    
    return 0;
}