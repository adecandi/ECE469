//
//	memory.c
//
//	Routines for dealing with memory management.

#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "memory.h"
#include "queue.h"

// num_pages = size_of_memory / size_of_one_page
// (MEM_MAX_SIZE >> MEM_L1FIELD_FIRST_BITNUM) / 32 = 16
static uint32 freemap[(MEM_MAX_SIZE >> MEM_L1FIELD_FIRST_BITNUM) / 32];
static uint32 pagestart;
static int ref_counters[MEM_MAX_SIZE >> MEM_L1FIELD_FIRST_BITNUM];
static int nfreepages;
static int freemapmax;

//----------------------------------------------------------------------
//
//	This silliness is required because the compiler believes that
//	it can invert a number by subtracting it from zero and subtracting
//	an additional 1.  This works unless you try to negate 0x80000000,
//	which causes an overflow when subtracted from 0.  Simply
//	trying to do an XOR with 0xffffffff results in the same code
//	being emitted.
//
//----------------------------------------------------------------------
static int negativeone = 0xFFFFFFFF;
static inline uint32 invert (uint32 n) {
  return (n ^ negativeone);
}

//----------------------------------------------------------------------
//
//	MemoryGetSize
//
//	Return the total size of memory in the simulator.  This is
//	available by reading a special location.
//
//----------------------------------------------------------------------
int MemoryGetSize() {
  return (*((int *)DLX_MEMSIZE_ADDRESS));
}


//----------------------------------------------------------------------
//
//	MemoryModuleInit
//
//	Initialize the memory module of the operating system.
//      Basically just need to setup the freemap for pages, and mark
//      the ones in use by the operating system as "VALID", and mark
//      all the rest as not in use.
//
//----------------------------------------------------------------------
void MemoryModuleInit() {
  int i;
  int maxpage = MemoryGetSize() / MEM_PAGESIZE;
  // 4-byte aline lastosadress then divide MEM_PAGESIZE
  uint32 ospages = (lastosaddress & 0x1FFFFC) / MEM_PAGESIZE;

  dbprintf('m', "MemoryModuleInit:  begin");
  printf("(MEM_MAX_SIZE >> MEM_L1FIELD_FIRST_BITNUM) / 32 = %d", (MEM_MAX_SIZE >> MEM_L1FIELD_FIRST_BITNUM) / 32);

  nfreepages = MEM_NUM_PAGES - ospages;
  pagestart = ospages + 1;
  freemapmax = (maxpage+31) / 32;

  // Initialize all to in use initially
  nfreepages = 0;
  for(i = 0; i < freemapmax; i++) {
    freemap[i] = 0;
  }

  // set reference counter for the os pages
  for(i = 0; i < pagestart; i++) {
    ref_counters[i] = 1;
  }

  // Go from the page start to the maxpage
  for(i = pagestart; i < maxpage; i++) {
    nfreepages += 1;
    MemoryEditFreemap(i, 1);
  }
  dbprintf('m', "Initialized %d free pages.\n", nfreepages);
}

void MemoryEditFreemap(int page, int val) {
  // get page and bit position
  uint32 index = page / 32;
  uint32 bit_position = page % 32;
  // set the val
  freemap[index] = (freemap[index] & invert(1 << bit_position)) | (val << bit_position);
}


//----------------------------------------------------------------------
//
// MemoryTranslateUserToSystem
//
//	Translate a user address (in the process referenced by pcb)
//	into an OS (physical) address.  Return the physical address.
//
//----------------------------------------------------------------------
uint32 MemoryTranslateUserToSystem (PCB *pcb, uint32 addr) {
  // Grabs the page and offset from the address
  uint32 page = MEM_ADDR2PAGE(addr);
  uint32 offset = MEM_ADDR2OFFS(addr);

  // Checks validity before returning
  if (pcb->pagetable[page] & MEM_PTE_VALID) {
    return ((pcb->pagetable[page] & MEM_MASK_PTE2PAGE) | offset);
  }
  return MEM_FAIL;
}


//----------------------------------------------------------------------
//
//	MemoryMoveBetweenSpaces
//
//	Copy data between user and system spaces.  This is done page by
//	page by:
//	* Translating the user address into system space.
//	* Copying all of the data in that page
//	* Repeating until all of the data is copied.
//	A positive direction means the copy goes from system to user
//	space; negative direction means the copy goes from user to system
//	space.
//
//	This routine returns the number of bytes copied.  Note that this
//	may be less than the number requested if there were unmapped pages
//	in the user range.  If this happens, the copy stops at the
//	first unmapped address.
//
//----------------------------------------------------------------------
int MemoryMoveBetweenSpaces (PCB *pcb, unsigned char *system, unsigned char *user, int n, int dir) {
  unsigned char *curUser;         // Holds current physical address representing user-space virtual address
  int		bytesCopied = 0;  // Running counter
  int		bytesToCopy;      // Used to compute number of bytes left in page to be copied

  while (n > 0) {
    // Translate current user page to system address.  If this fails, return
    // the number of bytes copied so far.
    curUser = (unsigned char *)MemoryTranslateUserToSystem (pcb, (uint32)user);

    // If we could not translate address, exit now
    if (curUser == (unsigned char *)0) break;

    // Calculate the number of bytes to copy this time.  If we have more bytes
    // to copy than there are left in the current page, we'll have to just copy to the
    // end of the page and then go through the loop again with the next page.
    // In other words, "bytesToCopy" is the minimum of the bytes left on this page 
    // and the total number of bytes left to copy ("n").

    // First, compute number of bytes left in this page.  This is just
    // the total size of a page minus the current offset part of the physical
    // address.  MEM_PAGESIZE should be the size (in bytes) of 1 page of memory.
    // MEM_ADDRESS_OFFSET_MASK should be the bit mask required to get just the
    // "offset" portion of an address.
    bytesToCopy = MEM_PAGESIZE - ((uint32)curUser & MEM_ADDR_OFFS_MASK);
    
    // Now find minimum of bytes in this page vs. total bytes left to copy
    if (bytesToCopy > n) {
      bytesToCopy = n;
    }

    // Perform the copy.
    if (dir >= 0) {
      bcopy (system, curUser, bytesToCopy);
    } else {
      bcopy (curUser, system, bytesToCopy);
    }

    // Keep track of bytes copied and adjust addresses appropriately.
    n -= bytesToCopy;           // Total number of bytes left to copy
    bytesCopied += bytesToCopy; // Total number of bytes copied thus far
    system += bytesToCopy;      // Current address in system space to copy next bytes from/into
    user += bytesToCopy;        // Current virtual address in user space to copy next bytes from/into
  }
  return (bytesCopied);
}

//----------------------------------------------------------------------
//
//	These two routines copy data between user and system spaces.
//	They call a common routine to do the copying; the only difference
//	between the calls is the actual call to do the copying.  Everything
//	else is identical.
//
//----------------------------------------------------------------------
int MemoryCopySystemToUser (PCB *pcb, unsigned char *from,unsigned char *to, int n) {
  return (MemoryMoveBetweenSpaces (pcb, from, to, n, 1));
}

int MemoryCopyUserToSystem (PCB *pcb, unsigned char *from,unsigned char *to, int n) {
  return (MemoryMoveBetweenSpaces (pcb, to, from, n, -1));
}

//---------------------------------------------------------------------
// MemoryPageFaultHandler is called in traps.c whenever a page fault 
// (better known as a "seg fault" occurs.  If the address that was
// being accessed is on the stack, we need to allocate a new page 
// for the stack.  If it is not on the stack, then this is a legitimate
// seg fault and we should kill the process.  Returns MEM_SUCCESS
// on success, and kills the current process on failure.  Note that
// fault_address is the beginning of the page of the virtual address that 
// caused the page fault, i.e. it is the vaddr with the offset zero-ed
// out.
//
// Note: The existing code is incomplete and only for reference. 
// Feel free to edit.
//---------------------------------------------------------------------
int MemoryPageFaultHandler(PCB *pcb) {
  // addresses to use
  uint32 user_stack_ptr = pcb->currentSavedFrame[PROCESS_STACK_USER_STACKPOINTER];
  uint32 fault_address = pcb->currentSavedFrame[PROCESS_STACK_FAULT];
  // corresponding pages for the addresses
  int pg_fault_address = MEM_ADDR2PAGE(fault_address);
  int genPage;

  user_stack_ptr &= 0x1FF000;

  dbprintf('m', "MemoryPageFaultHandler (%d): Begin1\n", GetPidFromAddress(pcb));

  // Compare fault address and user stack pointer
  if(fault_address < user_stack_ptr) {
    // True seg fault
    printf("Exiting PID %d: MemoryPageFaultHandler seg fault\n", GetPidFromAddress(pcb));
    dbprintf ('m', "MemoryPageFaultHandler (%d): seg fault addr=0x%x\n", GetPidFromAddress(pcb), fault_address);
    ProcessKill();
    return MEM_FAIL;
  } else {
    // Not a seg fault
    // Allocate a new page to use
    genPage = MemoryAllocPage();
    if(genPage == MEM_FAIL) {
      printf("FATAL: not enough free pages for %d\n", GetPidFromAddress(pcb));
      ProcessKill();
    }
    // Use the setup pte function
    pcb->pagetable[pg_fault_address] = MemorySetupPte(genPage);
    // Used to show a debug message that a new page has been allocated from the memorypagefault handler for part5
    dbprintf('z', "MemoryPageFaultHandler PID (%d): allocating new page (%d)\n", GetPidFromAddress(pcb), genPage);
    pcb->npages += 1;
    return MEM_SUCCESS;
  }
}


//---------------------------------------------------------------------
// You may need to implement the following functions and access them from process.c
// Feel free to edit/remove them
//---------------------------------------------------------------------

// Finds a free page in the freemap, allocates it and returns the number
int MemoryAllocPage(void) {
  int index = 0;
  uint32 bit_position;
  uint32 fm_segment;

  dbprintf('m', "MemoryAllocPage: function started\n");
  // If there are no freepages available return a memfail
  if(nfreepages == 0) {
    dbprintf('m', "MemoryAllocPage: no available pages\n");
    return MEM_FAIL;
  }

  // loop through the freemaps until you find one with available slots
  while(freemap[index] == 0) {
    index += 1;
    if(index >= freemapmax) {
      index = 0;
    }
  }
  // grab the freemap segment (32 bits of freemap)
  fm_segment = freemap[index];
  // loop to find the first available position in that segment
  for(bit_position = 0; (fm_segment & (1 << bit_position)) == 0; bit_position++){ }
  // set the specific value in that segment
  freemap[index]  &= invert(1 << bit_position);
  // grab the page number
  fm_segment = (index * 32) + bit_position; 
  dbprintf('m', "MemoryAllocPage: allocated memory from map=%d, page=%d\n", index, fm_segment);
  // Decrement nfreepages since it's in use
  nfreepages -= 1;
  // Set the reference counter to 1 since this is a newly used page
  ref_counters[fm_segment] = 1;
  return fm_segment; // page number on memory space
}

uint32 MemorySetupPte (uint32 page) {
  return ((page * MEM_PAGESIZE) | MEM_PTE_VALID);
}

void MemoryFreePageTableEntry(uint32 pte) {
  // Converts pagetable entry to page and then sends to memory free page function
  MemoryFreePage((pte & MEM_MASK_PTE2PAGE) / MEM_PAGESIZE);
}

void MemorySharePte (uint32 pte) {
  // convert PTE to page (the real fork process handles setting the read only bit)
  int page = ((pte & MEM_MASK_PTE2PAGE) / MEM_PAGESIZE);
  // increment the reference counter
  ref_counters[page] += 1;
  // debug message
  dbprintf('m', "MemorySharePte: page=%d count=%d\n", page, ref_counters[page]);
}

void MemoryFreePage(uint32 page) {
  // Now since multiple forks we need to check the ref_counters
  ref_counters[page] -= 1;
  if(ref_counters[page] > 0) {
    // if the ref_counters[page] isn't 0 yet, it's still used
    dbprintf('m', "MemoryFreePage: decrementing ref_counters[%d] to %d\n", page, ref_counters[page]);
  } else { // if the ref_counters is 0 we can actually free the page fully
    // flip the freemap bit to set the position to available
    MemoryEditFreemap(page, 1);
    // free page now open
    nfreepages += 1;
    dbprintf ('m',"Freeing page 0x%x, %d remaining.\n", page, nfreepages);
  }
}

void MemoryRopHandler(PCB * pcb) {
  // addresses to use
  uint32 fault_address = pcb->currentSavedFrame[PROCESS_STACK_FAULT];
  // corresponding pages for the addresses
  int pg_fault_address = MEM_ADDR2PAGE(fault_address);
  int parent_page = MEM_ADDR2PAGE(pcb->pagetable[pg_fault_address] & MEM_MASK_PTE2PAGE);
  int genPage;

  dbprintf('m', "MemoryRopHandler: Begin.\n");

  if(ref_counters[parent_page] > 1) {
    dbprintf('m', "Copying page %d to page %d\n", parent_page, pg_fault_address);
    // generate and setup a page
    genPage = MemoryAllocPage();
    pcb->pagetable[pg_fault_address] = MemorySetupPte (genPage);
    // do the copying
    bcopy((char *)(fault_address), (char *)(genPage * MEM_PAGESIZE), MEM_PAGESIZE);
    // decrement reference counter since one has its own
    ref_counters[parent_page] -= 1;
  } else {
    dbprintf('m', "MemoryRopHandler: Ref coutn is 1, inverting read only.\n");
    // Reference counter is only one, so reset the readonly
    pcb->pagetable[pg_fault_address] &= invert(MEM_PTE_READONLY);
  }
  dbprintf('m', "MemoryRopHandler: End.\n");
}

// Empty functions to get compile to work, referenced from piazza question
void* malloc(PCB* pcb, int memsize) {
  return NULL;
}

int mfree(PCB* pcb, void* ptr) {
  return -1;
}
