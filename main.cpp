#include "defines.h"
#include "trampoline.h"

#include <cstdio>
#include <iostream>

using namespace HOOKUP_NM;

int main()
{
	//uintptr_t target = reinterpret_cast<uintptr_t>(&printf);
	//uintptr_t dst = reinterpret_cast<uintptr_t>(&hookTest);

	//TrampolineHook hook(target, dst);
	//hook.Enable();
	//printf("12345");
	//hook.Disable();
	//printf("12345");

	return 0;
}