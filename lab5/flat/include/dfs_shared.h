#ifndef __DFS_SHARED__
#define __DFS_SHARED__

typedef struct dfs_superblock {
	char valid; //valid indicator
  	uint32 dfs_blocksize; //File System block size
  	uint32 dfs_numblocks; //Total number of file system blocks
  	uint32 dfs_start_block_inodes; //the starting file system block number for the array of inodes
  	uint32 num_inodes; //Number of inodes in the inode array
  	uint32 dfs_start_block_fbv; //the starting file system block number of the free block vector
  	uint32 dfs_start_block_data; // starting file system block number for data

} dfs_superblock;

#define DFS_BLOCKSIZE 1024  // Must be an integer multiple of the disk blocksize

typedef struct dfs_block {
  char data[DFS_BLOCKSIZE];
} dfs_block;


//inode needs to be 96 bytes, currently uses 53 bytes m max file name = 96-53 = 43
#define DFS_MAX_FILENAME_SIZE 44
typedef struct dfs_inode {
  // STUDENT: put inode structure internals here
  // IMPORTANT: sizeof(dfs_inode) MUST return 128 in order to fit in enough
  // inodes in the filesystem (and to make your life easier).  To do this, 
  // adjust the maximumm length of the filename until the size of the overall inode 
  // is 128 bytes.
	char inuse; //Indicates if inode is inuse or not
	uint32 filesize; //size of file that the inode reresents
	char filename[DFS_MAX_FILENAME_SIZE]; //Name of the file that the inode represents
	uint32 virt_blocks[10]; //Table of direct translations for first 10 virtual blocks
	uint32 indrect_block; //block num of file system, holds indirect address translations, only allocaed when needed
} dfs_inode;

#define DFS_MAX_FILESYSTEM_SIZE 0x1000000  // 16MB

#define DFS_NUM_INODES 192 
#define DFS_NUM_BLOCKS DFS_MAX_FILESYSTEM_SIZE / DFS_BLOCKSIZE;
#define DFS_FBV_MAX_NUM_WORDS DFS_NUM_BLOCKS / 32;


#define DFS_FAIL -1
#define DFS_SUCCESS 1



#endif
