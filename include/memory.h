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
	static Memory& GetInstance() noexcept
	{
		static Memory inst;
		return inst;
	}

	~Memory() = default;

private:
	Memory() = default;
	// no copy
	Memory(const Memory&) = delete;
	Memory& operator=(const Memory&) = delete;
	// no move
	Memory(const Memory&&) = delete;
	Memory& operator=(const Memory&&) = delete;

public:
	static VOID Patch(LPVOID address, LPBYTE data, size_t size) noexcept
	{
		DWORD oldProtect;
		VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
		__movsb((LPBYTE)address, (LPBYTE)data, size);
		VirtualProtect(address, size, oldProtect, &oldProtect);
	}

	template<typename T>
	static VOID Patch(uintptr_t address, T data) noexcept
	{
		Patch(address, &data, sizeof(T));
	}

	VOID UninitializeBuffer() noexcept;

	LPVOID AllocateBuffer(LPVOID origin) noexcept;

	VOID FreeBuffer(LPVOID buffer) noexcept;

	BOOL IsExecutableAddress(LPVOID pAddress) const noexcept;

private:
#if defined(_M_X64) || defined(__x86_64__)
	LPVOID FindPrevFreeRegion(LPVOID address, LPVOID minAddr, DWORD dwAllocationGranularity);

	LPVOID FindNextFreeRegion(LPVOID address, LPVOID maxAddr, DWORD dwAllocationGranularity);
#endif

	MemoryBlock* GetMemoryBlock(LPVOID origin) noexcept;

private:
	MemoryBlock* memory_blocks_{ nullptr };
};

_END_WINHOOKUPP_NM_