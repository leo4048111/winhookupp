#include "memory.h"

_START_WINHOOKUPP_NM_

Memory::~Memory() noexcept
{
	MemoryBlock *head = memory_blocks_;
	memory_blocks_ = nullptr;

	while (head)
	{
		MemoryBlock *next = head->next;
		VirtualFree(head, 0, MEM_RELEASE);
		head = next;
	}
}

LPVOID Memory::AllocateBuffer(LPVOID origin) noexcept
{
	MemorySlot *slot;
	MemoryBlock *block = GetMemoryBlock(origin);
	if (block == nullptr)
		return NULL;

	// Remove an unused slot from the list.
	slot = block->free;
	block->free = slot->next;
	block->usedCount++;

#ifdef _DEBUG
	// Fill the slot with INT3 for debugging.
	memset(slot, 0xCC, sizeof(MemorySlot));
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
#ifdef _DEBUG
			// Clear the released slot for debugging.
			memset(slot, 0x00, sizeof(MemorySlot));
#endif
			// Restore the released slot to the list.
			slot->next = block->free;
			block->free = slot;
			block->usedCount--;

			// Free if unused.
			if (block->usedCount == 0)
			{
				if (prevBlock)
					prevBlock->next = block->next;
				else
					memory_blocks_ = block->next;

				VirtualFree(block, 0, MEM_RELEASE);
			}

			break;
		}

		prevBlock = block;
		block = block->next;
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
		if (VirtualQuery((LPVOID)tryAddr, &mbi, sizeof(mbi)) == 0)
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
		if (VirtualQuery((LPVOID)tryAddr, &mbi, sizeof(mbi)) == 0)
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
	MemoryBlock *block;
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
	for (block = memory_blocks_; block != NULL; block = block->next)
	{
#if defined(_M_X64) || defined(__x86_64__)
		// Ignore the blocks too far.
		if ((uintptr_t)block < minAddr || (uintptr_t)block >= maxAddr)
			continue;
#endif
		// The block has at least one unused slot.
		if (block->free)
			return block;
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

			block = (MemoryBlock *)VirtualAlloc(
				alloc, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
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

			block = (MemoryBlock *)VirtualAlloc(
				alloc, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (block != NULL)
				break;
		}
	}
#else
	// In x86 mode, a memory block can be placed anywhere.
	block = (MemoryBlock *)VirtualAlloc(
		NULL, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#endif

	if (block != NULL)
	{
		// Build a linked list of all the slots.
		MemorySlot *slot = (MemorySlot *)block + 1;
		block->free = NULL;
		block->usedCount = 0;
		do
		{
			slot->next = block->free;
			block->free = slot;
			slot++;
		} while ((uintptr_t)slot - (uintptr_t)block <= MEMORY_BLOCK_SIZE - MEMORY_SLOT_SIZE);

		block->next = memory_blocks_;
		memory_blocks_ = block;
	}

	return block;
}

BOOL Memory::IsExecutableAddress(LPVOID pAddress) const noexcept
{
	MEMORY_BASIC_INFORMATION mi;
	VirtualQuery(pAddress, &mi, sizeof(mi));

	return (mi.State == MEM_COMMIT && (mi.Protect & PAGE_EXECUTE_FLAGS));
}

_END_WINHOOKUPP_NM_