#include "ostraps.h"
#include "dlxos.h"
#include "traps.h"
#include "queue.h"
#include "disk.h"
#include "dfs.h"
#include "synch.h"

static dfs_inode inodes[DFS_NUM_INODES]; 			// all inodes
static dfs_superblock sb; 							// superblock
static uint32 fbv[DFS_FBV_MAX_NUM_WORDS]; 			// Free block vector

static int negativeone = 0xFFFFFFFF;
static inline uint32 invert (uint32 n) {
  return (n ^ negativeone);
}

// You have already been told about the most likely places where you should use locks. You may use 
// additional locks if it is really necessary.
static lock_t fbv_lock;
static lock_t inode_lock;

// STUDENT: put your file system level functions below.
// Some skeletons are provided. You can implement additional functions.

inline void SetFBV (int p, int b)
{
  uint32  wd = p / 32;
  uint32  bitnum = p % 32;

  fbv[wd] = (fbv[wd] & invert(1 << bitnum)) | (b << bitnum);
  dbprintf ('Q', "Set fbv entry %d to 0x%x.\n", wd, fbv[wd]);
}

///////////////////////////////////////////////////////////////////
// Non-inode functions first
///////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------
// DfsModuleInit is called at boot time to initialize things and
// open the file system for use.
//-----------------------------------------------------------------

void DfsModuleInit() {
// You essentially set the file system as invalid and then open 
// using DfsOpenFileSystem().
	dbprintf('Q', "DfsModuleInit begin.\n");
	DfsInvalidate();
	fbv_lock = LockCreate();
	inode_lock = LockCreate();
	DfsOpenFileSystem();
	dbprintf('Q', "DfsModuleInit end.\n");
}

//-----------------------------------------------------------------
// DfsInavlidate marks the current version of the filesystem in
// memory as invalid.  This is really only useful when formatting
// the disk, to prevent the current memory version from overwriting
// what you already have on the disk when the OS exits.
//-----------------------------------------------------------------

void DfsInvalidate() {
// This is just a one-line function which sets the valid bit of the 
// superblock to 0.
	//printf("DfsInvalidate: superblock invalidated.\n");
	sb.valid = 0;
}

//-------------------------------------------------------------------
// DfsOpenFileSystem loads the file system metadata from the disk
// into memory.  Returns DFS_SUCCESS on success, and DFS_FAIL on 
// failure.
//-------------------------------------------------------------------

int DfsOpenFileSystem() {
	// Declarations
	disk_block diskb;
	dfs_block new_block;
	char * inodebytes;
	char * fbvbytes;
	int i;

	// Check that filesystem is not already open
	if (sb.valid) {
		printf("Error File System is already open.\n");
		return DFS_FAIL;
	}
	// Read superblock from disk.  Note this is using the disk read rather 
	// than the DFS read function because the DFS read requires a valid 
	// filesystem in memory already, and the filesystem cannot be valid 
	// until we read the superblock. Also, we don't know the block size 
	// until we read the superblock, either.
	if(DiskReadBlock(1, &diskb) == DISK_FAIL) {
		printf("Error reading from disk in DfsOpenFileSystem.\n");
		return DFS_FAIL;
	}

	// Copy the data from the block we just read into the superblock in memory

	bcopy(diskb.data, (char *)&sb, sizeof(sb));
	// All other blocks are sized by virtual block size:
	// Read inodes
	// sb should be valid at this point since sb was supposed to be valid on disk
	if(!sb.valid) {
		printf("DfsOpenFileSystem: Filesystem on disk is not valid\n");
		return DFS_FAIL;
	}
	inodebytes = (char *) inodes;
	for (i = sb.dfs_start_block_inodes; i < sb.dfs_start_block_fbv; i++) {
		//dbprintf('Q', "DfsOpenFileSystem: opening inodes. i=%d.\n", i);
		if (DfsReadBlock(i, &new_block) == DFS_FAIL) {
			printf("DfsOpenFileSystem: Error Reading dfs block for inodes.\n");
			return DFS_FAIL;
		}
		bcopy(new_block.data, &(inodebytes[(i - sb.dfs_start_block_inodes) * sb.dfs_blocksize]), sb.dfs_blocksize);
	}
	// Read free block vector
	fbvbytes = (char *) fbv;
	for (i = sb.dfs_start_block_fbv; i < sb.dfs_start_block_data; i++) {
		if (DfsReadBlock(i, &new_block) == DFS_FAIL) {
			printf("DfsOpenFileSystem: Error Reading dfs block for fbv.\n");
			return DFS_FAIL;
		}
		bcopy(new_block.data, &(fbvbytes[(i - sb.dfs_start_block_fbv) * sb.dfs_blocksize]), sb.dfs_blocksize);
	}

	// Change superblock to be invalid, write back to disk, then change 
	// it back to be valid in memory
	sb.valid = 0;
	bzero(diskb.data, DISK_BLOCKSIZE);
	bcopy((char *)&sb, diskb.data, sizeof(sb));
	if ( DiskWriteBlock(1, &diskb) == DISK_FAIL ) {
		printf("DfsOpenFileSystem: Error Writing superblock back to disk.\n");
		return DFS_FAIL;
	}
	sb.valid = 1;
	printf("DfsOpenFileSystem: Success. sb.valid=%d.\n", sb.valid);
	return DFS_SUCCESS;
}


//-------------------------------------------------------------------
// DfsCloseFileSystem writes the current memory version of the
// filesystem metadata to the disk, and invalidates the memory's 
// version.
//-------------------------------------------------------------------

int DfsCloseFileSystem() {
	// Declarations
	disk_block diskb;
	dfs_block new_block;
	char * bytes2write;
	int i;

	// Check that filesystem is not already closed
	if (!sb.valid) {
		printf("DfsCloseFileSystem: Error No File system open to close. sbvalid=%d\n", sb.valid);
		return DFS_FAIL;
	}

	//Write Inodes:
	bytes2write = (char *) inodes;
	for (i = sb.dfs_start_block_inodes; i < sb.dfs_start_block_fbv; i++) {
		bcopy(&(bytes2write[(i - sb.dfs_start_block_inodes) * sb.dfs_blocksize]), new_block.data, sb.dfs_blocksize);
		if ( DfsWriteBlock(i, &new_block) == DFS_FAIL ) {
			printf("DfsCloseFileSystem: Error writing inodes.\n");
			return DFS_FAIL;
		}
	}

	//Write fbv
	bytes2write = (char *) fbv;
	for (i = sb.dfs_start_block_fbv; i < sb.dfs_start_block_data; i++) {
		bcopy(&(bytes2write[(i - sb.dfs_start_block_fbv) * sb.dfs_blocksize]), new_block.data, sb.dfs_blocksize);
		if ( DfsWriteBlock(i, &new_block) == DFS_FAIL ) {
			printf("DfsCloseFileSystem: Error writing fbv.\n");
			return DFS_FAIL;
		}
	}

	//Write sb to disk
	bzero(diskb.data, DISK_BLOCKSIZE);
	bcopy((char *)&sb, diskb.data, sizeof(sb));
	if (DiskWriteBlock(1, &diskb) == DISK_FAIL) {
		printf("DfsCloseFileSystem: Error writing sb back to disk.\n");
		return DFS_FAIL;
	}
	sb.valid = 0; 
	printf("DfsCloseFileSystem: Success!\n");
	return DFS_SUCCESS;
}


//-----------------------------------------------------------------
// DfsAllocateBlock allocates a DFS block for use. Remember to use 
// locks where necessary.
//-----------------------------------------------------------------

uint32 DfsAllocateBlock() {
	//Initializations:
	int word;
	int bitnum;
	int i;
	uint32 v; //vector
	// Check that file system has been validly loaded into memory

	if (!sb.valid) {
		printf("DfsAllocateBlock: Error cannot Allocate block if file system is invalid");
		return DFS_FAIL;
	}
	// Find the first free block using the free block vector (FBV), mark it in use
	// Return handle to block
	//Wait for lock
	if(LockHandleAcquire(fbv_lock) != SYNC_SUCCESS) {
    	printf("DfsAllocateBlock bad lock acquire!\n");
    	return DFS_FAIL;
  	}

	for (i = 0; i < DFS_FBV_MAX_NUM_WORDS; i++) {
		if(fbv[i] != 0) {
    		break;
    	}
	}

	if(i == DFS_FBV_MAX_NUM_WORDS) {
		printf("DfsAllocateBlock: Could not allocate block\n");
		return DFS_FAIL;
	}

	//Mark the bit as in use
	v = fbv[i];
	for (bitnum = 0; (v & (1 << bitnum)) == 0; bitnum++) { }
    fbv[i]  &= invert(1 << bitnum);
   	//Find handle
    v = (i * 32) + bitnum;
    dbprintf('Q', "DfsAllocateBlock: allocated block from fbv=%d, vector=%d\n", i, v);
    //Release Lock
    LockHandleRelease(fbv_lock);
    //return handle
    return v; 
}


//-----------------------------------------------------------------
// DfsFreeBlock deallocates a DFS block.
//-----------------------------------------------------------------

int DfsFreeBlock(uint32 blocknum) {
	//Check if file system is valid
	if (!sb.valid) {
		printf("DfsAllocateBlock: Error cannot Allocate block if file system is invalid");
		return DFS_FAIL;
	}

	//Acquire Lock
	while(LockHandleAcquire(fbv_lock) != SYNC_SUCCESS) {}

	//Deallocate DFS block
	SetFBV(blocknum, 1);
	dbprintf('Q', "DfsFreeBlock: Freed block %d\n", blocknum);

	//Release Lock
	LockHandleRelease(fbv_lock);

	return DFS_SUCCESS;

}


//-----------------------------------------------------------------
// DfsReadBlock reads an allocated DFS block from the disk
// (which could span multiple physical disk blocks).  The block
// must be allocated in order to read from it.  Returns DFS_FAIL
// on failure, and the number of bytes read on success.  
//-----------------------------------------------------------------

int DfsReadBlock(uint32 blocknum, dfs_block *b) {
	//Initializations
	disk_block diskb;
	int m = sb.dfs_blocksize / DISK_BLOCKSIZE; // factor number of disk blocks per dfs block
	int i;

	//Check if file system is valid
	if (!sb.valid) {
		printf("DfsReadBlock: Error cannot Allocate block if file system is invalid.\n");
		return DFS_FAIL;
	}

	//Check if block is allocated
	if (fbv[blocknum / 32] & (1 << (blocknum % 32))) {
		printf("DfsReadBlock: Error blocknumber %d is not allocated.\n", blocknum);
		//printf("DfsReadBlock: fbv[blocknum / 32]= %x, (1 << (blocknum mod 32))= %x.\n", fbv[blocknum / 32], (1 << (blocknum % 32)));
		//printf("DfsReadBlock: (fbv[blocknum / 32] & (1 << (blocknum mod 32))) = %d.\n", !(fbv[blocknum / 32] & (1 << (blocknum % 32))));
		return DFS_FAIL;
	}

	//Read the block from disk
	for (i = 0; i < m; i++) {
		bzero(diskb.data, DISK_BLOCKSIZE);
		if (DiskReadBlock(blocknum * m + i, &diskb) == DISK_FAIL) {
			printf("DfsReadBlock: Error could not read disk block.\n");
			return DFS_FAIL;
		}
		bcopy(diskb.data, &(b->data[i * DISK_BLOCKSIZE]), DISK_BLOCKSIZE);
	}

	return sb.dfs_blocksize;

}


//-----------------------------------------------------------------
// DfsWriteBlock writes to an allocated DFS block on the disk
// (which could span multiple physical disk blocks).  The block
// must be allocated in order to write to it.  Returns DFS_FAIL
// on failure, and the number of bytes written on success.  
//-----------------------------------------------------------------

int DfsWriteBlock(uint32 blocknum, dfs_block *b){
	//Initializations
	disk_block diskb;
	int dbsz;
	int m; // factor number of disk blocks per dfs block
	int i;
	int num_writ = 0;

	dbsz = DiskBytesPerBlock();
	m = sb.dfs_blocksize / dbsz;

	//Check if file system is valid
	if (!sb.valid) {
		printf("DfsAllocateBlock: Error cannot Allocate block if file system is invalid.\n");
		return DFS_FAIL;
	}

	//Check if block is allocated
	if (fbv[blocknum/32] & (1 << (blocknum % 32))) {
		printf("DfsWriteBlock: Error blocknumber %d is not allocated.\n", blocknum);
		return DFS_FAIL;
	}

	//Write block to disk
	for (i = 0; i < m; i++) {
		bzero(diskb.data, dbsz);
		bcopy(&(b->data[i * dbsz]), diskb.data, dbsz);
		if (DiskWriteBlock(blocknum * m + i, &diskb) == DISK_FAIL) {
			printf("DfsWriteBlock: Error could not write to disk. blocknum=%d, m=%d, i=%d\n", blocknum, m, i);
			return DFS_FAIL;
		}
		num_writ += dbsz;
	}

	//return bytes wriiten
	return num_writ;
}


////////////////////////////////////////////////////////////////////////////////
// Inode-based functions
////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------
// DfsInodeFilenameExists looks through all the inuse inodes for 
// the given filename. If the filename is found, return the handle 
// of the inode. If it is not found, return DFS_FAIL.
//-----------------------------------------------------------------

uint32 DfsInodeFilenameExists(char *filename) {
	//Initializations
	int i;

	//Check if file system is valid
	if (!sb.valid) {
		printf("DfsInodeFilenameExists: Error inode filename does not exist if file system is invalid\n");
		return DFS_FAIL;
	}

	//Check if filename exists
	for (i = 0; i < DFS_NUM_INODES; i++) {
		if (inodes[i].inuse) {
			if (dstrncmp(filename, inodes[i].filename, DFS_MAX_FILENAME_SIZE) == 0) {
				return i;
			}
		}
	}
	//printf("DfsInodeFilenameExists: Error Inode filename was not found\n");
	return DFS_FAIL;

}


//-----------------------------------------------------------------
// DfsInodeOpen: search the list of all inuse inodes for the 
// specified filename. If the filename exists, return the handle 
// of the inode. If it does not, allocate a new inode for this 
// filename and return its handle. Return DFS_FAIL on failure. 
// Remember to use locks whenever you allocate a new inode.
//-----------------------------------------------------------------

uint32 DfsInodeOpen(char *filename) {
	//Initializations
	uint32 i = 0; 
	uint32 inhandle;
	//Check if file system is valid
	if (!sb.valid) {
		printf("DfsInodeOpen: Error cannot open inode if file system is invalid\n");
		return DFS_FAIL;
	}

	//Check if filename exists already, return handle
	if ((inhandle = DfsInodeFilenameExists(filename)) != DFS_FAIL) {
		return inhandle;
	//Otherwise, allocate new inode for filename and return new handle
	} else {
		//Acquire Lock
		// dbprintf('Q', "DfsInodeOpen: Acquire lock.\n");
		while (LockHandleAcquire(inode_lock) != SYNC_SUCCESS);
		//Allocate new inode for filename
		// dbprintf('Q', "DfsInodeOpen: Allocate new inode for filename.\n");
		/*(while (inodes[i].inuse == 0) {
			i += 1;
			if (i >= DFS_NUM_INODES) {
				printf("DfsInodeOpen: Error all inodes inuse, cannot open inode\n");
				return DFS_FAIL;
			}
		}*/
		for(i=0; i < DFS_NUM_INODES; i++) {
			if(!inodes[i].inuse) {
				inodes[i].inuse = 1;
				inodes[i].filesize = 0;
				dstrncpy(inodes[i].filename, filename, DFS_MAX_FILENAME_SIZE);
				break;
			}
		}
		//Release lock
		LockHandleRelease(inode_lock);
		dbprintf('Q', "DfsInodeOpen: Opened inode:%d, inuse=%d\n", i, inodes[i].inuse);
		return i; 
	}
	return DFS_FAIL;
}


//-----------------------------------------------------------------
// DfsInodeDelete de-allocates any data blocks used by this inode, 
// including the indirect addressing block if necessary, then mark 
// the inode as no longer in use. Use locks when modifying the 
// "inuse" flag in an inode.Return DFS_FAIL on failure, and 
// DFS_SUCCESS on success.
//-----------------------------------------------------------------

int DfsInodeDelete(uint32 handle) {
	//Initializations
	int i = 0; 
	//uint32 inhandle;
	dfs_block ind_block;
	uint32 indirect_table[sb.dfs_blocksize / 4];
	//Check if file system is valid
	if (!sb.valid) {
		printf("DfsInodeDelete: Error cannot delete inode if file system is invalid\n");
		return DFS_FAIL;
	}

	//Check if inode is inuse, inode must be inuse for function to work
	if (!inodes[handle].inuse) {
		printf("DfsInodeDelete: Error cannot delete inode if it is not inuse\n");
		return DFS_FAIL;
	}

	//Deallocate blocks represented by Inodes
	
	//Deallocate inderect addressing block
	//Read the inderect block from disk into a dfs block to make a table of indirect addresses to free
	if (DfsReadBlock(inodes[handle].indirect_block, &ind_block) == DFS_FAIL) {
		printf("DfsInodeDelete: Error could not read inderct block into dfs block\n");
		return DFS_FAIL;
	}

	//Free/Deallocate all blocks referenced by the indirect address
	bcopy (ind_block.data, (char *)indirect_table, sb.dfs_blocksize);
	while(indirect_table[i] != 0) {
		if(DfsFreeBlock(indirect_table[i]) == DFS_FAIL) {
			printf("DfsInodeDelete: could not free an address in indirect table. i=%d\n", i);
			return DFS_FAIL;
		}
		i++;
	}

	if(inodes[handle].indirect_block != 0) {
		if(DfsFreeBlock(inodes[handle].indirect_block) == DFS_FAIL) {
			printf("DfsInodeDelete: could not free indirect block.\n");
			return DFS_FAIL;
		}
	}

	//Set indirect address to 0
	inodes[handle].indirect_block = 0;

	//Free blocks referenced by virt address blocks
	for (i = 0; i < 10; i++) {
		if(inodes[handle].virt_blocks[i] != 0) {
			if (DfsFreeBlock(inodes[handle].virt_blocks[i]) == DFS_FAIL) {
				printf("DfsInodeDelete: Error could not deallocate block # %d, referenced by virtual address block", inodes[handle].virt_blocks[i]);
				return DFS_FAIL;
			}
		}

		//Set translation back to 0
		inodes[handle].virt_blocks[i] = 0;
	}

	//clear filesize
	inodes[handle].filesize = 0;
	bzero(inodes[handle].filename, DFS_MAX_FILENAME_SIZE);

	//Acquire lock for changing inode use
	if(LockHandleAcquire(inode_lock) != SYNC_SUCCESS) {
    	printf("DfsFreeBlock bad lock acquire!\n");
    	return DFS_FAIL;
	}

	inodes[handle].inuse = 0;

	if(LockHandleRelease(inode_lock) != SYNC_SUCCESS) {
    	printf("DfsFreeBlock bad lock release!\n");
    	return DFS_FAIL;
	}

	return DFS_SUCCESS;
}


//-----------------------------------------------------------------
// DfsInodeReadBytes reads num_bytes from the file represented by 
// the inode handle, starting at virtual byte start_byte, copying 
// the data to the address pointed to by mem. Return DFS_FAIL on 
// failure, and the number of bytes read on success.
//-----------------------------------------------------------------

int DfsInodeReadBytes(uint32 handle, void *mem, int start_byte, int num_bytes) {
	//Initializations
	//int i; 
	dfs_block new_block;
	//uint32 indirect_table[sb.dfs_blocksize / 4];
	int curr_byte = start_byte;
	int bytes_read = 0;
	int bytestoread;
	uint32 filesys_blocknum;

	//Check if file system is valid
	if (!sb.valid) {
		printf("DfsInodeReadBytes: Error cannot read num_bytes into mem if file system is invalid\n");
		return DFS_FAIL;
	}

	//Check if inode is valid
	if (!inodes[handle].inuse) {
		printf("DfsInodeReadBytes: Error cannot read num_bytes into mem if inode is not in use\n");
		return DFS_FAIL;
	}

	//Read bytes from the file represented, starting at the start byte, going until num bytes
	while (bytes_read < num_bytes) {
		//get block number from translation
		if ( (filesys_blocknum = DfsInodeTranslateVirtualToFilesys(handle, curr_byte / sb.dfs_blocksize)) == DFS_FAIL) {
			printf("DfsInodeReadBytes: Error trying to translate virt_blocknum to filesys_blocknum\n");
			return DFS_FAIL;
		}
		//read block into a new dfs block
		if (DfsReadBlock(filesys_blocknum, &new_block) == DFS_FAIL) {
			printf("DfsInodeReadBytes: Error reading into dfs block\n");
			return DFS_FAIL;
		}

		//Caluclate bytes being read from block into mem
		bytestoread = sb.dfs_blocksize - (curr_byte % sb.dfs_blocksize);

		//check if bytestoread  + the current byte count goes over the num_bytes, if so adjust bytestoread to reach num_bytes
		if ((bytestoread + bytes_read) > num_bytes) {
			bytestoread = num_bytes - bytes_read;
		}

		//Copy bytes to read from dfs block into mem
		//bcopy(&(new_block.data[curr_byte % sb.dfs_blocksize]), &(mem[bytes_read]), bytestoread);
		bcopy(&(new_block.data[curr_byte % sb.dfs_blocksize]), (char *) mem + bytes_read, bytestoread);

		//update curr_byte and bytes_read
		curr_byte += bytestoread;
		bytes_read += bytestoread;
	}

	return bytes_read;
}


//-----------------------------------------------------------------
// DfsInodeWriteBytes writes num_bytes from the memory pointed to 
// by mem to the file represented by the inode handle, starting at 
// virtual byte start_byte. Note that if you are only writing part 
// of a given file system block, you'll need to read that block 
// from the disk first. Return DFS_FAIL on failure and the number 
// of bytes written on success.
//-----------------------------------------------------------------

int DfsInodeWriteBytes(uint32 handle, void *mem, int start_byte, int num_bytes) {
	//Initializations
	//int i; 
	dfs_block new_block;
	int curr_byte = start_byte;
	int bytes_written = 0;
	int bytestowrite;
	int virt_blocknum;

	//Check if file system is valid
	if (!sb.valid) {
		printf("DfsInodeWriteBytes: Error cannot write num_bytes into mem if file system is invalid\n");
		return DFS_FAIL;
	}

	//Check if inode is valid
	if (!inodes[handle].inuse) {
		printf("DfsInodeWriteBytes: inode (%d) not inuse (%d).\n", handle, inodes[handle].inuse);
		return DFS_FAIL;
	}

	// dbprintf('Q', "DfsInodeWriteBytes: entering while loop. num_bytes=%d, handle=%d.\n", num_bytes, handle);
	//write bytes from the file represented, starting at the start byte, going until num bytes
	while (bytes_written < num_bytes) {
		if((virt_blocknum = DfsInodeAllocateVirtualBlock(handle, curr_byte / sb.dfs_blocksize)) == DFS_FAIL) {
			printf("DfsInodeWriteBytes: Error cannot allocate virt block.\n");
			return DFS_FAIL;
		}
		dbprintf('Q', "DfsInodeWriteBytes: allocate virt_block returned: %d.\n", virt_blocknum);

		bytestowrite = sb.dfs_blocksize - (curr_byte % sb.dfs_blocksize);
		if(bytestowrite == sb.dfs_blocksize) {
			// first part is block alligned
			if((bytes_written + bytestowrite) > num_bytes) {
				bytestowrite = bytestowrite - ((bytes_written + bytestowrite) - num_bytes);
				if(DfsReadBlock(virt_blocknum, &new_block) == DFS_FAIL) {
					printf("DfsInodeWriteBytes: DfsReadBlock failed.\n");
					return DFS_FAIL;
				}
			}
		} else {
			// woah, not block aligned
			if((bytes_written + bytestowrite) > num_bytes) {
				bytestowrite = bytestowrite - ((bytes_written + bytestowrite) - num_bytes);
			}
			if(DfsReadBlock(virt_blocknum, &new_block) == DFS_FAIL) {
					printf("DfsInodeWriteBytes: DfsReadBlock failed.\n");
					return DFS_FAIL;
			}
		}

		bcopy((char *) mem + bytes_written, &(new_block.data[curr_byte % sb.dfs_blocksize]), bytestowrite);


		if (DfsWriteBlock(virt_blocknum, &new_block) == DFS_FAIL) {
			printf("DfsInodeWriteBytes: Error Writing dfs block back to disk after writing in from mem\n");
			return DFS_FAIL;
		}
		curr_byte += bytestowrite;
		bytes_written += bytestowrite;
		dbprintf('Q', "DfsInodeWriteBytes: curr_byte = %d, bytes_written: %d, num_bytes: %d, fs_blck=%d.\n", curr_byte, bytes_written, num_bytes, virt_blocknum);
	}

	if (inodes[handle].filesize < start_byte + bytes_written) {
		inodes[handle].filesize = start_byte + bytes_written;
	}

	return bytes_written;

}


//-----------------------------------------------------------------
// DfsInodeFilesize simply returns the size of an inode's file. 
// This is defined as the maximum virtual byte number that has 
// been written to the inode thus far. Return DFS_FAIL on failure.
//-----------------------------------------------------------------

uint32 DfsInodeFilesize(uint32 handle) {
	//Check if file system is valid
	if (!sb.valid) {
		printf("DfsInodeWriteBytes: Error cannot write num_bytes into mem if file system is invalid\n");
		return DFS_FAIL;
	}

	//Check if inode is valid
	if (!inodes[handle].inuse) {
		printf("DfsInodeWriteBytes: Error cannot write num_bytes into mem if inode is not in use\n");
		return DFS_FAIL;
	}

	return inodes[handle].filesize;

}


//-----------------------------------------------------------------
// DfsInodeAllocateVirtualBlock allocates a new filesystem block 
// for the given inode, storing its blocknumber at index 
// virtual_blocknumber in the translation table. If the 
// virtual_blocknumber resides in the indirect address space, and 
// there is not an allocated indirect addressing table, allocate it. 
// Return DFS_FAIL on failure, and the newly allocated file system 
// block number on success.
//-----------------------------------------------------------------

uint32 DfsInodeAllocateVirtualBlock(uint32 handle, uint32 virtual_blocknum) {
	//Initializations
	dfs_block new_block;
	uint32 indirect_table[sb.dfs_blocksize / 4];
	
	dbprintf('Q', "DfsInodeAllocateVirtualBlock: Begin. Handle=%d, vblock=%d\n", handle, virtual_blocknum);
	//Check if file system is valid
	if (!sb.valid) {
		printf("DfsInodeAllocateVirtualBlock: Error Cannot allocate virt block if file system is invalid\n");
		return DFS_FAIL;
	}

	//Check if inode is valid
	if (!inodes[handle].inuse) {
		printf("DfsInodeAllocateVirtualBlock: Error Cannot allocate virt block if inode is not inuse\n");
		return DFS_FAIL;
	}

	//Decide if virt_blocknumber is in virt_block table or indirect
	//Virt_block table allocate
	if (virtual_blocknum < 10) {
		dbprintf('Q', "DfsInodeAllocateVirtualBlock: virt_block table.\n");
		if ((inodes[handle].virt_blocks[virtual_blocknum] = DfsAllocateBlock()) == DFS_FAIL) {
			printf("DfsInodeAllocateVirtualBlock: Error Cannot allocate virtual block from v-table\n");
			return DFS_FAIL;
		}
		return inodes[handle].virt_blocks[virtual_blocknum];
	}
	//indirect_table allocate
	else {
		dbprintf('Q', "DfsInodeAllocateVirtualBlock: indirect table.\n");
		//check if indirect_block has been allocated, if not allocate it
		if (inodes[handle].indirect_block == 0) {
			dbprintf('Q', "DfsInodeAllocateVirtualBlock: indirect_block not allocated.\n");
			if ((inodes[handle].indirect_block = DfsAllocateBlock()) == DFS_FAIL) {
				printf("DfsInodeAllocateVirtualBlock: Error Cannot allocate indirect_block\n");
				return DFS_FAIL;
			}
			dbprintf('Q', "DfsInodeAllocateVirtualBlock: bzero and DfsWriteBlock.\n");
			//0 disk for allocation
			bzero(new_block.data, sb.dfs_blocksize);
			if (DfsWriteBlock(inodes[handle].indirect_block, &new_block)) {
				printf("DfsInodeAllocateVirtualBlock: Error writing 0 to disk for indirect_addressing. Block=%d.\n", inodes[handle].indirect_block);
				return DFS_FAIL;
			}
			dbprintf('Q', "DfsInodeAllocateVirtualBlock: allocation complete.\n");
		}

		dbprintf('Q', "DfsInodeAllocateVirtualBlock: read indirect block.\n");
		//Read indirect block from disk, copy into indirect table, allocate properly, write back to disk
		if (DfsReadBlock(inodes[handle].indirect_block, &new_block) == DFS_FAIL) {
			printf("DfsInodeAllocateVirtualBlock: Error could not read inderct block into dfs block for allocation\n");
			return DFS_FAIL;
		}
		dbprintf('Q', "DfsInodeAllocateVirtualBlock: bcopy and DfsAllocateBlock.\n");
		bcopy (new_block.data, (char *)indirect_table, sb.dfs_blocksize);
		virtual_blocknum -= 10;
		if ((indirect_table[virtual_blocknum] = DfsAllocateBlock()) == DFS_FAIL) {
			printf("DfsInodeAllocateVirtualBlock: Error Cannot allocate indirect_table at virt_blocknum");
			return DFS_FAIL;
		}

		//write back into disk
		dbprintf('Q', "DfsInodeAllocateVirtualBlock: write back into disk.\n");
		bcopy((char *)indirect_table, new_block.data, sb.dfs_blocksize);
		if (DfsWriteBlock(inodes[handle].indirect_block, &new_block) == DFS_FAIL) {
			printf("DfsInodeAllocateVirtualBlock: Error writing updated indirect_adress table back to disk\n");
			return DFS_FAIL;
		}

		return indirect_table[virtual_blocknum];
	}
}



//-----------------------------------------------------------------
// DfsInodeTranslateVirtualToFilesys translates the 
// virtual_blocknum to the corresponding file system block using 
// the inode identified by handle. Return DFS_FAIL on failure.
//-----------------------------------------------------------------

uint32 DfsInodeTranslateVirtualToFilesys(uint32 handle, uint32 virtual_blocknum) {
	//initializations
	dfs_block new_block;
	uint32 indirect_table[sb.dfs_blocksize / 4];

	//Check if file system is valid
	if (!sb.valid) {
		printf("DfsInodeTranslateVirtualToFilesys: Error cannot translate virt_blocknum to fsystem block if file system is invalid\n");
		return DFS_FAIL;
	}

	//Check if inode is valid
	if (!inodes[handle].inuse) {
		printf("DfsInodeTranslateVirtualToFilesys: Error cannot translate virt_blocknum to fsystem block if inode is not in use\n");
		return DFS_FAIL;
	}

	//Decide if its in virt_block table or indirect
	//Virt_block translation
	if (virtual_blocknum < 10) {
		return inodes[handle].virt_blocks[virtual_blocknum];
	}
	//indirect translation
	else {
			//check if indirect block has been allocated (must be allocated for translation)
			if (inodes[handle].indirect_block == 0) {return 0;}
			//Read the inderect block from disk into a dfs block to make a table of indirect addresses to check for translation
			bzero(new_block.data, sb.dfs_blocksize);
			if (DfsReadBlock(inodes[handle].indirect_block, &new_block) == DFS_FAIL) {
				printf("DfsInodeTranslateVirtualToFilesys: Error could not read inderct block into dfs block for translation\n");
				return DFS_FAIL;
			}
			bcopy (new_block.data, (char *)indirect_table, sb.dfs_blocksize);
			//Go through indirect table, check if virt_blocknum matches and return the translation
			virtual_blocknum -= 10;
			if (virtual_blocknum >= (sb.dfs_blocksize/4)) {
				printf("DfsInodeTranslateVirtualToFilesys: Error virt blocknum is out of index of indirect addressing\n");
				return DFS_FAIL;
			}
			return indirect_table[virtual_blocknum];
	}


	printf("DfsInodeTranslateVirtualToFilesys: Wierd Error");
	return DFS_FAIL;

}

