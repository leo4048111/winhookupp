#include "memory.h"

_START_HOOKUP_NM_

VOID Memory::UninitializeBuffer() noexcept {
	MemoryBlock* head = memory_blocks_;
	memory_blocks_ = nullptr;

	while (head)
	{
		MemoryBlock* next = head->next;
		VirtualFree(head, 0, MEM_RELEASE);
		head = next;
	}
}

LPVOID Memory::AllocateBuffer(LPVOID origin) noexcept
{
	MemorySlot* slot;
	MemoryBlock* block = GetMemoryBlock(origin);
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
	MemoryBlock* block = memory_blocks_;
	MemoryBlock* prevBlock = nullptr;
	uintptr_t targetBlock = ((uintptr_t)buffer / MEMORY_BLOCK_SIZE) * MEMORY_BLOCK_SIZE;

	while (block)
	{
		if ((uintptr_t)block == targetBlock)
		{
			MemorySlot* slot = (MemorySlot*)buffer;
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

Memory::MemoryBlock* Memory::GetMemoryBlock(LPVOID origin) noexcept
{
	MemoryBlock* block;
#if defined(_M_X64) || defined(__x86_64__)
	uintptr_t minAddr;
	uintptr_t maxAddr;

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	minAddr = (uintptr_t)si.lpMinimumApplicationAddress;
	maxAddr = (uintptr_t)si.lpMaximumApplicationAddress;

	// origin ¡À 512MB
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
		if (pBlock->pFree != NULL)
			return pBlock;
	}

#if defined(_M_X64) || defined(__x86_64__)
	// Alloc a new block above if not found.
	{
		LPVOID pAlloc = origin;
		while ((ULONG_PTR)pAlloc >= minAddr)
		{
			pAlloc = FindPrevFreeRegion(pAlloc, (LPVOID)minAddr, si.dwAllocationGranularity);
			if (pAlloc == NULL)
				break;

			pBlock = (PMEMORY_BLOCK)VirtualAlloc(
				pAlloc, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (pBlock != NULL)
				break;
		}
	}

	// Alloc a new block below if not found.
	if (pBlock == NULL)
	{
		LPVOID pAlloc = origin;
		while ((ULONG_PTR)pAlloc <= maxAddr)
		{
			pAlloc = FindNextFreeRegion(pAlloc, (LPVOID)maxAddr, si.dwAllocationGranularity);
			if (pAlloc == NULL)
				break;

			pBlock = (PMEMORY_BLOCK)VirtualAlloc(
				pAlloc, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (pBlock != NULL)
				break;
		}
	}
#else
	// In x86 mode, a memory block can be placed anywhere.
	pBlock = (PMEMORY_BLOCK)VirtualAlloc(
		NULL, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#endif

	if (pBlock != NULL)
	{
		// Build a linked list of all the slots.
		PMEMORY_SLOT pSlot = (PMEMORY_SLOT)pBlock + 1;
		pBlock->pFree = NULL;
		pBlock->usedCount = 0;
		do
		{
			pSlot->pNext = pBlock->pFree;
			pBlock->pFree = pSlot;
			pSlot++;
		} while ((ULONG_PTR)pSlot - (ULONG_PTR)pBlock <= MEMORY_BLOCK_SIZE - MEMORY_SLOT_SIZE);

		pBlock->pNext = g_pMemoryBlocks;
		g_pMemoryBlocks = pBlock;
	}

	return pBlock;
}


_END_HOOKUP_NM_