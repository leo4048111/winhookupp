# winhookupp
[![License](https://img.shields.io/badge/License-BSD%202--Clause-orange.svg)](https://opensource.org/licenses/BSD-2-Clause)

A cpp multi-method API Hooking library for x86/x86-64 Windows, basically simple class wrappers and interfaces for hooking methods which i pasted from various sources.

# Supported hooking methods
+ **VEH hook:** Trigger exception with page guard violation, then intercept the exception with our VehHandler and change instruction pointer to detoured function.
+ **Trampoline hook:** Modifies opcode to jmp to hook and allocates a trampoline for jmp back.
+ **INT3VEH hook:** Alike VEH hook, except that exception is triggered by patching the first byte of target function to 0xCC(int 3)
+ **VMT:** TODO

# Download this project
```
git clone git@github.com:leo4048111/winhookupp.git --recurse-submodules
```
# Building
+ Installed Visual Studio
+ Generate solution with tests
```
cmake . -G "Visual Studio 17 2022" -A [win32/x64]
```
+ Generate solution library only
```
cmake . -G "Visual Studio 17 2022" -A [win32/x64] -DBUILD_TESTS=OFF
```

# Usage
+ Example trampoline hooking(Interfaces of hooking methods are the same): 
```cpp
// include header for a hooking method
#include "trampoline.h"

int main(int argc, char** argv) {
    using namespace WINHOOKUPP_NM;

    // define a hook instance
    Trampoline tramp;
    // enabling hooking
    LPVOID origin; // a pointer to original target function(nullptr if calling original target is not possible after hooking)
    tramp.Enable(&TargetFunction, &YourDetouredFunction, &origin);

    // calling the target function, your detoured function should be called instead
    TargetFunction(...);

    // calling the original function
    if(origin != nullptr) {
        decltype(TargetFunction)* pOrigin = reinterpret_cast<decltype(TargetFunction)*>(origin);
        pOrigin(...);
    }

    // disabling hooking(note that hook will be automatically disabled if the hook instance is deconstructed)
    tramp.Disable();
}
```
+ See example and unit-tests for more detailed usages.

# Credits
+ https://github.com/TsudaKageyu/minhook
+ https://github.com/hoangprod/LeoSpecial-VEH-Hook
