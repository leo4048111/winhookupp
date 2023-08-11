// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "defines.h"

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include <cstdint>
#include <Windows.h>

_START_WINHOOKUPP_NM_

// Size of each memory block. (= page size of VirtualAlloc)
#define MEMORY_BLOCK_SIZE 0x1000

// Size of each memory slot.
#if defined(_M_X64) || defined(__x86_64__)
#define MEMORY_SLOT_SIZE 64
#else
#define MEMORY_SLOT_SIZE 32
#endif

// Max range for seeking a memory block. (= 1024MB)
#define MAX_MEMORY_RANGE 0x40000000

// Memory protection flags to check the executable address.
#define PAGE_EXECUTE_FLAGS \
    (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)

class Memory
{
public:
	typedef struct MemorySlot
	{
		union
		{
			MemorySlot* next{nullptr};
			uint8_t buffer[MEMORY_SLOT_SIZE];
		};
	} MemorySlot;

	// Memory block info. Placed at the head of each block.
	typedef struct MemoryBlock
	{
		MemoryBlock* next{nullptr};
		MemorySlot* free{nullptr};         // First element of the free slot list.
		size_t usedCount{ 0 };
	} MemoryBlock;

public:
#ifndef WINHOOKUPP_EXTERNAL_USAGE
	static Memory& GetInstance() noexcept
	{
		static Memory inst;
		return inst;
	}
#else 
	static Memory& GetInstance(HANDLE hProcess) noexcept
	{
		static Memory inst(hProcess);
		return inst;
	}
#endif
	~Memory();

private:
#ifndef WINHOOKUPP_EXTERNAL_USAGE
	Memory() = default;
#else 
	Memory() = delete;
	Memory(HANDLE hProcess) noexcept : hProcess_(hProcess) {};
#endif
	// no copy
	Memory(const Memory&) = delete;
	Memory& operator=(const Memory&) = delete;
	// no move
	Memory(const Memory&&) = delete;
	Memory& operator=(const Memory&&) = delete;

public:
#ifndef WINHOOKUPP_EXTERNAL_USAGE
	template<typename T>
	T Read(LPVOID src) noexcept
	{
		T data;
		Read(&data, (LPBYTE)src, sizeof(T));
		return data;
	}

	VOID Read(LPVOID dst, LPBYTE src, size_t size) noexcept
	{
		DWORD oldProtect;
		VirtualProtect(src, size, PAGE_READWRITE, &oldProtect);
		__movsb((LPBYTE)dst, (LPBYTE)src, size);
		VirtualProtect(src, size, oldProtect, &oldProtect);
	}

	VOID Copy(LPVOID dst, LPBYTE src, size_t size) noexcept
	{
		Patch(dst, src, size);
	}

	VOID Patch(LPVOID address, LPBYTE data, size_t size) noexcept
	{
		DWORD oldProtect;
		VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
		__movsb((LPBYTE)address, (LPBYTE)data, size);
		VirtualProtect(address, size, oldProtect, &oldProtect);
	}

	template<typename T>
	VOID Patch(LPVOID address, T data) noexcept
	{
		Patch(address, (LPBYTE)&data, sizeof(T));
	}
#else 
	template<typename T>
	T ReadEx(LPVOID src) noexcept
	{
		T data;
		ReadEx(&data, src, sizeof(T));
		return data;
	}

	VOID ReadEx(LPVOID dst, LPVOID src, size_t size) noexcept
	{
		DWORD oldProtect;
		VirtualProtectEx(hProcess_, src, size, PAGE_READWRITE, &oldProtect);
		ReadProcessMemory(hProcess_, dst, src, size, nullptr);
		VirtualProtectEx(hProcess_, src, size, oldProtect, &oldProtect);
	}

	VOID CopyEx(LPVOID dst, LPBYTE src, size_t size) noexcept
	{
		ReadEx(dst, src, size);
	}

	VOID PatchEx(LPVOID address, LPBYTE data, size_t size) noexcept
	{
		DWORD oldProtect;
		VirtualProtectEx(hProcess_, address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
		WriteProcessMemory(hProcess_, (LPBYTE)address, (LPBYTE)data, size, nullptr);
		VirtualProtectEx(hProcess_, address, size, oldProtect, &oldProtect);
	}

	template<typename T>
	VOID PatchEx(LPVOID address, T data) noexcept
	{
		PatchEx(address, (LPBYTE)&data, sizeof(T));
	}
#endif

	bool IsExecutableAddress(LPVOID pAddress) noexcept;

	bool IsCodePadding(LPBYTE pInst, size_t size) noexcept;

	LPVOID AllocateBuffer(LPVOID origin) noexcept;

	VOID FreeBuffer(LPVOID buffer) noexcept;

private:
#if defined(_M_X64) || defined(__x86_64__)
	LPVOID FindPrevFreeRegion(LPVOID address, LPVOID minAddr, DWORD dwAllocationGranularity);

	LPVOID FindNextFreeRegion(LPVOID address, LPVOID maxAddr, DWORD dwAllocationGranularity);
#endif

	MemoryBlock* GetMemoryBlock(LPVOID origin) noexcept;

private:
	MemoryBlock* memory_blocks_{ nullptr };

#ifdef WINHOOKUPP_EXTERNAL_USAGE
	HANDLE hProcess_{ nullptr };
#endif	
};

_END_WINHOOKUPP_NM_