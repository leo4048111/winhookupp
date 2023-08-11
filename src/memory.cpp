// Copyright 2023 leo4048111. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "memory.h"

_START_WINHOOKUPP_NM_

Memory::~Memory() noexcept
{
	MemoryBlock *head = memory_blocks_;
	memory_blocks_ = nullptr;

	while (head)
	{
		MemoryBlock* next = nullptr;
#ifdef WINHOOKUPP_EXTERNAL_USAGE
		next = ReadEx<MemoryBlock*>(&head->next);
		VirtualFreeEx(hProcess_, head, 0, MEM_RELEASE);
#else
		next = head->next;
		VirtualFree(head, 0, MEM_RELEASE);
#endif
		head = next;
	}
}

LPVOID Memory::AllocateBuffer(LPVOID origin) noexcept
{
	MemorySlot *slot = nullptr;
	MemoryBlock *block = GetMemoryBlock(origin);
	if (block == nullptr)
		return NULL;

	// Remove an unused slot from the list.

#ifdef WINHOOKUPP_EXTERNAL_USAGE
	slot = ReadEx<MemorySlot*>(&block->free);
	MemorySlot* free = ReadEx<MemorySlot*>(&slot->next);
	PatchEx(&block->free, free);
	size_t oldSize = ReadEx<size_t>(&block->usedCount);
	PatchEx(&block->usedCount, (size_t)(oldSize + 1));
#else
	slot = block->free;
	block->free = slot->next;
	block->usedCount++;
#endif

	return slot;
}

VOID Memory::FreeBuffer(LPVOID buffer) noexcept
{
	MemoryBlock *block = memory_blocks_;
	MemoryBlock *prevBlock = nullptr;
	uintptr_t targetBlock = ((uintptr_t)buffer / MEMORY_BLOCK_SIZE) * MEMORY_BLOCK_SIZE;

	while (block)
	{
		if ((uintptr_t)block == targetBlock)
		{
			MemorySlot *slot = (MemorySlot *)buffer;

			// Restore the released slot to the list.
#ifdef WINHOOKUPP_EXTERNAL_USAGE
			MemorySlot* free = ReadEx<MemorySlot*>(&block->free);
			PatchEx(&slot->next, free);
			PatchEx(&block->free, slot);
			size_t oldSize = ReadEx<size_t>(&block->usedCount);
			PatchEx(&block->usedCount, (size_t)(oldSize - 1));
#else
			slot->next = block->free;
			block->free = slot;
			block->usedCount--;
#endif

			size_t useCount = 0;
#ifdef WINHOOKUPP_EXTERNAL_USAGE
			useCount = ReadEx<size_t>(&block->usedCount);
#else
			useCount = block->usedCount;
#endif
			// Free if unused.
			if (useCount == 0)
			{
				if (prevBlock)
				{
#ifdef WINHOOKUPP_EXTERNAL_USAGE
					MemoryBlock* next = ReadEx<MemoryBlock*>(&block->next);
					PatchEx(&prevBlock->next, next);
#else
					prevBlock->next = block->next;
#endif
				}
				else
				{
#ifdef WINHOOKUPP_EXTERNAL_USAGE
					memory_blocks_ = ReadEx<MemoryBlock*>(&block->next);
#else
					memory_blocks_ = block->next;
#endif
				}
#ifdef WINHOOKUPP_EXTERNAL_USAGE
				VirtualFreeEx(hProcess_, block, 0, MEM_RELEASE);
#else
				VirtualFree(block, 0, MEM_RELEASE);
#endif
			}

			break;
		}
		prevBlock = block;
#ifdef WINHOOKUPP_EXTERNAL_USAGE
		block = ReadEx<MemoryBlock*>(&block->next);
#else
		block = block->next;
#endif
	}
}

#if defined(_M_X64) || defined(__x86_64__)
LPVOID Memory::FindPrevFreeRegion(LPVOID address, LPVOID minAddr, DWORD dwAllocationGranularity)
{
	uintptr_t tryAddr = (uintptr_t)address;

	// Round down to the allocation granularity.
	tryAddr -= tryAddr % dwAllocationGranularity;

	// Start from the previous allocation granularity multiply.
	tryAddr -= dwAllocationGranularity;

	while (tryAddr >= (uintptr_t)minAddr)
	{
		MEMORY_BASIC_INFORMATION mbi;
		SIZE_T result = 0;

#ifdef WINHOOKUPP_EXTERNAL_USAGE
		result = VirtualQueryEx(hProcess_, (LPVOID)tryAddr, &mbi, sizeof(mbi));
#else
		result = VirtualQuery((LPVOID)tryAddr, &mbi, sizeof(mbi));
#endif

		if (result == 0)
			break;

		if (mbi.State == MEM_FREE)
			return (LPVOID)tryAddr;

		if ((uintptr_t)mbi.AllocationBase < dwAllocationGranularity)
			break;

		tryAddr = (uintptr_t)mbi.AllocationBase - dwAllocationGranularity;
	}

	return NULL;
}

LPVOID Memory::FindNextFreeRegion(LPVOID address, LPVOID maxAddr, DWORD dwAllocationGranularity)
{
	uintptr_t tryAddr = (uintptr_t)address;

	// Round down to the allocation granularity.
	tryAddr -= tryAddr % dwAllocationGranularity;

	// Start from the next allocation granularity multiply.
	tryAddr += dwAllocationGranularity;

	while (tryAddr <= (uintptr_t)maxAddr)
	{
		MEMORY_BASIC_INFORMATION mbi;

		SIZE_T result = 0;

#ifdef WINHOOKUPP_EXTERNAL_USAGE
		result = VirtualQueryEx(hProcess_, (LPVOID)tryAddr, &mbi, sizeof(mbi));
#else
		result = VirtualQuery((LPVOID)tryAddr, &mbi, sizeof(mbi));
#endif

		if (result == 0)
			break;

		if (mbi.State == MEM_FREE)
			return (LPVOID)tryAddr;

		tryAddr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;

		// Round up to the next allocation granularity.
		tryAddr += dwAllocationGranularity - 1;
		tryAddr -= tryAddr % dwAllocationGranularity;
	}

	return NULL;
}
#endif

Memory::MemoryBlock *Memory::GetMemoryBlock(LPVOID origin) noexcept
{
	MemoryBlock *block = memory_blocks_;
#if defined(_M_X64) || defined(__x86_64__)
	uintptr_t minAddr;
	uintptr_t maxAddr;

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	minAddr = (uintptr_t)si.lpMinimumApplicationAddress;
	maxAddr = (uintptr_t)si.lpMaximumApplicationAddress;

	// origin +- 512MB
	if ((uintptr_t)origin > MAX_MEMORY_RANGE && minAddr < (uintptr_t)origin - MAX_MEMORY_RANGE)
		minAddr = (uintptr_t)origin - MAX_MEMORY_RANGE;

	if (maxAddr > (uintptr_t)origin + MAX_MEMORY_RANGE)
		maxAddr = (uintptr_t)origin + MAX_MEMORY_RANGE;

	// Make room for MEMORY_BLOCK_SIZE bytes.
	maxAddr -= MEMORY_BLOCK_SIZE - 1;
#endif

	// Look the registered blocks for a reachable one.
	for (;;)
	{
		if (block == nullptr) break;

#if defined(_M_X64) || defined(__x86_64__)
		// Ignore the blocks too far.
		if ((uintptr_t)block < minAddr || (uintptr_t)block >= maxAddr)
			continue;
#endif
		// The block has at least one unused slot.
		MemorySlot* slot = nullptr;
#ifndef WINHOOKUPP_EXTERNAL_USAGE
		slot = block->free;
#else
		slot = ReadEx<MemorySlot*>(&block->free);
#endif
		if (slot)
			return block;

#ifndef WINHOOKUPP_EXTERNAL_USAGE
		block = block->next;
#else
		block = ReadEx<MemoryBlock*>(&block->next);
#endif
	}

#if defined(_M_X64) || defined(__x86_64__)
	// Alloc a new block above if not found.
	{
		LPVOID alloc = origin;
		while ((uintptr_t)alloc >= minAddr)
		{
			alloc = FindPrevFreeRegion(alloc, (LPVOID)minAddr, si.dwAllocationGranularity);
			if (alloc == NULL)
				break;

#ifdef WINHOOKUPP_EXTERNAL_USAGE
			block = (MemoryBlock*)VirtualAllocEx(
				hProcess_, alloc, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
			block = (MemoryBlock *)VirtualAlloc(
				alloc, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#endif
			if (block != NULL)
				break;
		}
	}

	// Alloc a new block below if not found.
	if (block == NULL)
	{
		LPVOID alloc = origin;
		while ((uintptr_t)alloc <= maxAddr)
		{
			alloc = FindNextFreeRegion(alloc, (LPVOID)maxAddr, si.dwAllocationGranularity);
			if (alloc == NULL)
				break;

#ifdef WINHOOKUPP_EXTERNAL_USAGE
			block = (MemoryBlock*)VirtualAllocEx(
				hProcess_, alloc, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
			block = (MemoryBlock *)VirtualAlloc(
				alloc, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#endif
			if (block != NULL)
				break;
		}
	}
#else
	// In x86 mode, a memory block can be placed anywhere.
#ifdef WINHOOKUPP_EXTERNAL_USAGE
	block = (MemoryBlock*)VirtualAllocEx(
		hProcess_, NULL, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
	block = (MemoryBlock *)VirtualAlloc(
		NULL, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#endif
#endif

	if (block != NULL)
	{
		// Build a linked list of all the slots.
		MemorySlot *slot = (MemorySlot *)block + 1;
#ifdef WINHOOKUPP_EXTERNAL_USAGE
		PatchEx(&block->free, (MemorySlot*)NULL);
		PatchEx(&block->usedCount, (size_t)0);
#else
		block->free = NULL;
		block->usedCount = 0;
#endif
		do
		{
#ifdef WINHOOKUPP_EXTERNAL_USAGE
			MemorySlot* free = ReadEx<MemorySlot*>(&block->free);
			PatchEx(&slot->next, free);
			PatchEx(&block->free, slot);
#else
			slot->next = block->free;
			block->free = slot;
#endif
			slot++;
		} while ((uintptr_t)slot - (uintptr_t)block <= MEMORY_BLOCK_SIZE - MEMORY_SLOT_SIZE);

#ifdef WINHOOKUPP_EXTERNAL_USAGE
		PatchEx(&block->next, memory_blocks_);
#else
		block->next = memory_blocks_;
#endif
		memory_blocks_ = block;
	}

	return block;
}

bool Memory::IsExecutableAddress(LPVOID pAddress) noexcept
{
	MEMORY_BASIC_INFORMATION mi;
	ZeroMemory(&mi, sizeof(mi));

#ifdef WINHOOKUPP_EXTERNAL_USAGE
	VirtualQueryEx(hProcess_, pAddress, &mi, sizeof(mi));
#else
	VirtualQuery(pAddress, &mi, sizeof(mi));
#endif

	return (mi.State == MEM_COMMIT && (mi.Protect & PAGE_EXECUTE_FLAGS));
}

bool Memory::IsCodePadding(LPBYTE pInst, size_t size) noexcept
{
	size_t i;
	BYTE byte;

#ifdef WINHOOKUPP_EXTERNAL_USAGE
	byte = ReadEx<BYTE>(&pInst[0]);
#else
	byte = Read<BYTE>(&pInst[0]);
#endif

	if (byte != 0x00 && byte != 0x90 && byte != 0xCC)
		return false;

	for (i = 1; i < size; ++i)
	{
		BYTE curByte;

#ifdef WINHOOKUPP_EXTERNAL_USAGE
		curByte = ReadEx<BYTE>(&pInst[i]);
#else
		curByte = Read<BYTE>(&pInst[i]);
#endif

		if (curByte != byte)
			return false;
	}
	return true;
}

_END_WINHOOKUPP_NM_