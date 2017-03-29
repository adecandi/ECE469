#ifndef	_memory_constants_h_
#define	_memory_constants_h_

//------------------------------------------------
// #define's that you are given:
//------------------------------------------------

// We can read this address in I/O space to figure out how much memory
// is available on the system.
#define	DLX_MEMSIZE_ADDRESS	0xffff0000

// Return values for success and failure of functions
#define MEM_SUCCESS 1
#define MEM_FAIL -1

//--------------------------------------------------------
// Put your constant definitions related to memory here.
// Be sure to prepend any constant names with "MEM_" so 
// that the grader knows they are defined in this file.

//--------------------------------------------------------

// Bit position of the least significant bit of the level 1 page number field 
// in a virtual address.
#define MEM_L1FIELD_FIRST_BITNUM 12
// The maximum allowable address in the virtual address space. Note that this 
// is not the 4-byte-aligned address, but rather the actual maximum address 
// (it should end with 0xF).
#define MAX_VIRTUAL_ADDRESS 0xFFFF
// Use a maximum physical memory size of 2MB
#define MEM_MAX_SIZE 0x200000

// PTE codes
#define MEM_PTE_READONLY 0x4
#define MEM_PTE_DIRTY 0x2
#define MEM_PTE_VALID 0x1

// Calculated constants given in "Bitwise tricks" section of the lab 
#define MEM_PAGESIZE (0x1 << MEM_L1FIELD_FIRST_BITNUM)
#define MEM_PAGE_OFFSET_MASK (MEM_PAGESIZE - 1)
#define MEM_L1PAGETABLE_SIZE ((MEM_MAX_VIRTUAL_ADDRESS + 1) >> MEM_L1FIELD_FIRST_BITNUM)
#define MEM_MASK_PTE2PAGE (~(MEM_PTE_READONLY | MEM_PTE_DIRTY | MEM_PTE_VALID))
#define MEM_NUM_PAGES (MEM_MAX_SIZE / MEM_PAGESIZE)
#define MEM_ADDR_OFFS_MASK (MEM_PAGESIZE - 1)

// Conversions
#define MEM_ADDR2PAGE(address) ((address) >> MEM_L1FIELD_FIRST_BITNUM)
#define MEM_ADDR2OFFS(address) ((address) & MEM_ADDR_OFFS_MASK)

#endif	// _memory_constants_h_
