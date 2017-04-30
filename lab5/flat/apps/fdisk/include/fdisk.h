#ifndef __FDISK_H__
#define __FDISK_H__

typedef unsigned int uint32;

#include "dfs_shared.h" // This gets us structures and #define's from main filesystem driver

#define FDISK_INODE_BLOCK_START 1 // Starts after super block (which is in file system block 0, physical block 1)
#define FDISK_INODE_NUM_BLOCKS 16 // Number of file system blocks to use for inodes
#define FDISK_NUM_INODES  192
#define FDISK_FBV_BLOCK_START FDISK_INODE_NUM_BLOCKS + FDISK_INODE_BLOCK_START;
#define FDISK_BOOT_FILESYSTEM_BLOCKNUM 0 // Where the boot record and superblock reside in the filesystem

#ifndef NULL
#define NULL (void *)0x0
#endif

//STUDENT: define additional parameters here, if any
#define FDISK_NUM_BLOCKS DFS_MAX_FILESYSTEM_SIZE / DFS_BLOCKSIZE 
#define DFS_FBV_MAX_NUM_WORDS FDISK_NUM_BLOCKS / 32;
#endif
