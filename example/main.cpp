// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <windows.h>
#include <iostream>

#include "veh.h"
#include "trampoline.h"
#include "vmt.h"

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
       
    class DummyBase {
    public:
        virtual DECLSPEC_NOINLINE bool DummyTarget() noexcept {
            ::std::cout << "DummyBase::DummyTarget() is called..." << ::std::endl;
            return true;
        }

        virtual DECLSPEC_NOINLINE bool DummyTarget2() noexcept {
            ::std::cout << "DummyBase::DummyTarget2() is called..." << ::std::endl;
            return true;
        }
    };

    class DummyDerived : public DummyBase {
    public:
        virtual DECLSPEC_NOINLINE bool DummyTarget() noexcept override {
			::std::cout << "DummyDerived::DummyTarget() is called..." << ::std::endl;
			return true;
		}

        virtual DECLSPEC_NOINLINE bool DummyTarget2() noexcept override {
            ::std::cout << "DummyDerived::DummyTarget2() is called..." << ::std::endl;
            return true;
        }
    };

    DECLSPEC_NOINLINE bool DetouredDummyTarget() noexcept {
		::std::cout << "DetouredDummyTarget() is called..." << ::std::endl;
		return false;
	}
}

#ifdef WINHOOKUPP_EXTERNAL_USAGE
int main(int argc, char** argv) {
    using namespace WINHOOKUPP_NM;
    return 0;
}
#else
int main(int argc, char** argv) {
    using namespace WINHOOKUPP_NM;

    bool result = true;

    // veh hooking
    Veh veh;
    result = veh.Enable(&WriteConsole, &DetouredWriteConsole);

    // calling detoured function
    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwChars;
    TCHAR buf[] = "Original WriteConsole() is called...\n";
    int len = lstrlen(buf);
    WriteConsole(hConsoleOutput, buf, len, &dwChars, nullptr);

    // calling original function
    // not possible in veh hooking

    // restore hook
    result = veh.Disable();

    // trampoline hooking
    Trampoline trampoline;
    using WriteConsole_t = decltype(WriteConsole);
    LPVOID origin;
    result = trampoline.Enable(&WriteConsole, &DetouredWriteConsole, &origin);

    // calling detoured function
    WriteConsole(hConsoleOutput, buf, len, &dwChars, nullptr);

    // calling original function
    WriteConsole_t* pOrigin = reinterpret_cast<WriteConsole_t*>(origin);
    pOrigin(hConsoleOutput, buf, len, &dwChars, nullptr);

    // restore hook
    result = trampoline.Disable();

    // vmt hooking, passing &classname::vmethod or address to virtual method as target are both acceptable
    Vmt vmt;
    DummyDerived dummy;
    DummyBase* pDummy = &dummy;
    
    // passing &classname::vmethod as target(which is very likely not the real address of
    // the virtual method, but the address of the vcall thunk)
    auto x = &DummyDerived::DummyTarget2;
    LPVOID target = *reinterpret_cast<LPVOID*&>(x);
    origin = nullptr;

    // a pointer to instance is required for vmt hooks
    result = vmt.Enable(target, &::DetouredDummyTarget, pDummy, &origin);

    // calling detoured function
    pDummy->DummyTarget2();

    //restore hook
    vmt.Disable();

    // passing real address of the virtual method as target
    constexpr size_t index = 1;
    target = (LPVOID)((uintptr_t)pDummy + index * sizeof(uintptr_t));

    // a pointer to instance is required for vmt hooks
    result = vmt.Enable(target, &::DetouredDummyTarget, pDummy, &origin);

    // calling detoured function
    pDummy->DummyTarget2();

    //restore hook
    vmt.Disable();

    return 0;
}
#endif