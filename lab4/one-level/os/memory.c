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
static int freemapmax;
static uint32 freemap[16];
static uint32 pagestart;
static int nfreepages;

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

  nfreepages = MEM_NUM_PAGES - ospages;
  pagestart = ospages + 1;
  freemapmax = (maxpage+31) / 32;

  // Initialize all to in use initially
  nfreepages = 0;
  for(i = 0; i < freemapmax; i++) {
    freemap[i] = 0;
  }

  // Go from the page start to the maxpage
  for(i = pagestart; i < maxpage; i++) {
    nfreepages += 1;
    MemoryEditFreemap(i, 1);
  }
  dbprintf('m', "Initialized %d free pages.\n", nfreepages);
}

void MemoryEditFreemap(int page, int val) {
  uint32 index = page / 32;
  uint32 bit_position = page % 32;
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
  uint32 page = MEM_ADDR2PAGE(addr);
  uint32 offset = MEM_ADDR2OFFS(addr);

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
  uint32 fault_addr = pcb->currentSavedFrame[PROCESS_STACK_FAULT];
  // corresponding pages for the addresses
  int pg_fault_addr = MEM_ADDR2PAGE(fault_addr);
  int pg_user_stack_ptr = MEM_ADDR2PAGE(user_stack_ptr);
  int vpage;
  int newPage;

  dbprintf('m', "MemoryPageFaultHandler (%d): Begin1\n", GetPidFromAddress(pcb));
  if(fault_addr < user_stack_ptr) {
    printf("Exiting PID %d: MemoryPageFaultHandler seg fault\n", GetPidFromAddress(pcb));
    dbprintf ('m', "MemoryPageFaultHandler (%d): seg fault addr=0x%x\n", GetPidFromAddress(pcb), fault_addr);
    ProcessKill();
    return MEM_FAIL;
  }
  else {
    dbprintf('z', "MemoryPageFaultHandler allocating new page\n");
    vpage = pg_fault_addr;
    newPage = MemoryAllocPage();
    if(newPage == MEM_FAIL) {
      printf("FATAL: not enough free pages for %d\n", GetPidFromAddress(pcb));
      ProcessKill();
    }
    pcb->pagetable[vpage] = MemorySetupPte(newPage);
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
  static int mapnum = 0;
  int bitnum;
  uint32 vector;

  dbprintf('m', "MemoryAllocPage: function started nfreepages=%d\n", nfreepages);
  if(nfreepages == 0) {
    return MEM_FAIL;
  }
  while(freemap[mapnum] == 0) {
    mapnum += 1;
    if(mapnum >= freemapmax) {
      mapnum = 0;
    }
  }
  vector = freemap[mapnum];
  for(bitnum = 0; (vector & (1 << bitnum)) == 0; bitnum++){}
  freemap[mapnum]  &= invert(1 << bitnum);
  vector = (mapnum * 32) + bitnum; // use same var to store free page number
  dbprintf('m', "MemoryAllocPage: allocated memory from map=%d, page=%d\n", mapnum, vector);
  nfreepages -= 1;
  return vector; // page number on memory space
}

uint32 MemorySetupPte (uint32 page) {
  return ((page * MEM_PAGESIZE) | MEM_PTE_VALID);
}

void MemoryFreePageTableEntry(uint32 pte) {
  MemoryFreePage((pte & MEM_MASK_PTE2PAGE) / MEM_PAGESIZE);
}

void MemoryFreePage(uint32 page) {
  MemoryEditFreemap(page, 1);
  nfreepages += 1;
  dbprintf ('m',"Freeing page 0x%x, %d remaining.\n", page, nfreepages);
}

void* malloc(PCB* pcb, int memsize) {
  return NULL;
}

int mfree(PCB* pcb, void* ptr) {
  return -1;
}