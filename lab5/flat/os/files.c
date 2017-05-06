#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "dfs.h"
#include "files.h"
#include "synch.h"

// You have already been told about the most likely places where you should use locks. You may use 
// additional locks if it is really necessary.
static file_descriptor fds[FILE_MAX_OPEN_FILES];
static lock_t fds_lock; 

// STUDENT: put your file-level functions here

void FileModuleInit() {
	int i;
	for (i = 0; i < FILE_MAX_OPEN_FILES; i++) {
		fds[0].inuse = 0;
	}
	fds_lock = LockCreate();
}

//open the given filename with one of three possible modes: "r", "w", or "rw". Return FILE_FAIL on failure 
//(e.g., when a process tries to open a file that is already open for another process),
//and the handle of a file descriptor on success. Remember to use locks whenever you allocate a new file descriptor.
int FileOpen(char *filename, char *mode) {
	//Initializations:
	int i;
	uint32 inode_handle;

	//Check if already open:
	for (i = 0; i < FILE_MAX_OPEN_FILES; i++) {
		//check if inuse
		if (fds[i].inuse) {
			//compare filenames
			if (dstrncmp(filename, fds[i].filename, DFS_MAX_FILENAME_SIZE) == 0) {
				printf("FileOpen: Error file is already opened by another process\n");
				return FILE_FAIL;
			}
		}
	}


	//Retrieve inode handle, fails if it does not exist
	if (( inode_handle = DfsInodeFilenameExists(filename)) == DFS_FAIL) {
		printf("FileOpen: Error Could not open file for inode_handle dfs inode handle does not exist\n");
		return FILE_FAIL;
	}

	//Find the next available file descriptor, mark as inuse and copy its filename to the descriptor and everything else to 0 (for now)
	//Acquire Lock before allocating new file descriptor
	while (LockHandleAcquire(fds_lock) != SYNC_SUCCESS) {}
	i = 0;
	while (fds[i].inuse == 1) {
		if (i >= FILE_MAX_OPEN_FILES) {
			printf("FileOpen: Error all files are inuse\n");
			return FILE_FAIL;
		}
	}
	fds[i].inuse = 1;
	dstrcpy(fds[i].filename, filename);
	fds[i].mode = 0;
	fds[i].pos = 0;
	fds[i].eof_flag = 0;

	//Mark its mode
	//check read
	if (dstrncmp("r", mode, 2) == 0) {
		fds[i].mode = 1;
	} 
	//check write
	else if (dstrncmp("w", mode, 2) == 0) {
		fds[i].mode = 2;
	}
	//check read/write
	else if (dstrncmp("rw", mode, 3) == 0) {
		fds[i].mode = 3;
	}
	//Otherwise no valid mode selected, return fail
	else {
		printf("FileOpen: Error Mode passed through into fileopen as parameter is invalid\n");
		return FILE_FAIL;
	}
	LockHandleRelease(fds_lock);
    return i;
	
}

//close the given file descriptor handle. Return FILE_FAIL on failure, and FILE_SUCCESS on success
int FileClose(int handle) {
	//Initializations:
	//Check if file is already closed
	if (!fds[i].inuse) {
		printf("FileClose: Error File is already closed \n");
		return FILE_FAIL; 
	}

	fds[i].inuse = 0;
	return FILE_SUCCESS;
}

//read num_bytes from the open file descriptor identified by handle. Return FILE_FAIL on failure or upon reaching end of file, 
//and the number of bytes read on success. If end of file is reached, the end-of-file flag in the file descriptor should be set
int FileRead(int handle, void *mem, int num_bytes) {
	//Initializations:
	int filesize;
	//Check if file is inuse
	if (!fds[i].inuse) {
		printf("FileRead: Error could not read file already closed \n");
		return FILE_FAIL; 
	}

	//Check if its in the correct mode
	if (fds[i].mode != 1) {
		printf("FileRead: Error could not read file because mode is not in read\n");
		return FILE_FAIL;
	}

	//Check if if it reached eof already
	if (fds[i].eof_flag) {
		printf("FileRead: Error could not read file because file already reached eof (eof flag is set)\n");
		return FILE_FAIL;
	}

	//maximum number of bytes that can be read or written at any time by the file functions is 4096 bytes, check if num_bytes surpasses this
	if (num_bytes > FILE_MAX_READWRITE_BYTES) {
		printf("FileRead: Error could not read file because the number of bytes wanting to be read exceeds the max # of bytes to be read possible (4096)\n");
		return FILE_FAIL;
	}

	//Get the filesize
	if ( (filesize = DfsInodeFilesize(fds[handle].inode_handle)) == DFS_FAIL ) {
		printf("FileRead: Error could not read file because the filesize could not be found from inode handle\n");
		return FILE_FAIL:
	}

	//Check if it reaches end of file
	if (fds[i].pos + num_bytes > filesize) {
		printf("Reading until the end of file\n");
		num_bytes = filsize - fds[i].pos;
		fds[i].eof_flag = 1;
	}

	//Read the bytes, return fail if failed
	if ((DfsInodeReadBytes(fds[handle].inode_handle, mem, fds[i].pos, num_bytes)) == DFS_FAIL) {
		printf("FileRead: Error could not read file because DfsInodeReadBytes failed\n");
		return FILE_FAIL;
	}

	fds[i].pos += num_bytes;
	//check if eof reach, return failed if so
	if (fds[i].eof_flag) {
		printf("FileRead: Error End of file reached could not read all bytes specified originall\n");
		return FILE_FAIL;
	}

	return num_bytes;

}


//write num_bytes to the open file descriptor identified by handle. Return FILE_FAIL on failure, and the number of bytes written on success.
int FileWrite(int handle, void *mem, int num_bytes) {
	//Initializations:
	int filesize;
	//Check if file is inuse
	if (!fds[i].inuse) {
		printf("FileWrite: Error could not write file already closed \n");
		return FILE_FAIL; 
	}

	//Check if its in the correct mode
	if (fds[i].mode != 2) {
		printf("FileWrite: Error could not write file because mode is not in write\n");
		return FILE_FAIL;
	}

	//Check if if it reached eof already
	if (fds[i].eof_flag) {
		printf("FileWrite: Error could not write file because file already reached eof (eof flag is set)\n");
		return FILE_FAIL;
	}

	//maximum number of bytes that can be read or written at any time by the file functions is 4096 bytes, check if num_bytes surpasses this
	if (num_bytes > FILE_MAX_READWRITE_BYTES) {
		printf("FileWrite: Error could not write file because the number of bytes wanting to be read exceeds the max # of bytes to be written possible (4096)\n");
		return FILE_FAIL;
	}

	//Get the filesize
	if ( (filesize = DfsInodeFilesize(fds[handle].inode_handle)) == DFS_FAIL ) {
		printf("FileWrite: Error could not write file because the filesize could not be found from inode handle\n");
		return FILE_FAIL:
	}

	//Check if it reaches end of file
	if (fds[i].pos + num_bytes > filesize) {
		printf("writing until the end of file\n");
		num_bytes = filsize - fds[i].pos;
		fds[i].eof_flag = 1;
	}

	//Read the bytes, return fail if failed
	if ((DfsInodeWriteBytes(fds[handle].inode_handle, mem, fds[i].pos, num_bytes)) == DFS_FAIL) {
		printf("FileWrite: Error could not write file because DfsInodeWriteBytes failed\n");
		return FILE_FAIL;
	}

	fds[i].pos += num_bytes;
	//check if eof reach, return failed if so
	if (fds[i].eof_flag) {
		printf("FileWrite: Error End of file reached could not write all bytes specified originally\n");
		return FILE_FAIL;
	}

	return num_bytes;
}

//Seek num_bytes within the file descriptor identified by handle, from the location specified by from_where. 
//There are three possible values for from_where: FILE_SEEK_CUR (seek relative to the current position), 
//FILE_SEEK_SET (seek relative to the beginning of the file), and FILE_SEEK_END (seek relative to the end of the file). Any seek operation will clear the eof flag.
int FileSeek(int handle, int num_bytes, int from_where) {
	//Initializations:
	int filesize;
	//Check if file is inuse
	if (!fds[i].inuse) {
		printf("FileSeek: Error could not seek file already closed \n");
		return FILE_FAIL; 
	}

	//Check for valid from_where Seek parameter
	if ((from_where != FILE_SEEK_SET) || (from_where != FILE_SEEK_END) || (from_where != FILE_SEEK_CUR)) {
		printf("FileSeek: Error from_where parameter is invalid for File Seek\n"); 
	}

	//Clear eof flag and get filesize
	fds[handle].eof_flag = 0;
	if ( (filesize = DfsInodeFilesize(fds[handle].inode_handle)) == DFS_FAIL ) {
		printf("FileSeek: Error could not seek file because the filesize could not be found from inode handle\n");
		return FILE_FAIL:
	}

	//SEEK SET
	if (from_where == FILE_SEEK_SET) {
		if ((num_bytes < 0) || (num_bytes > filsize)) {
			printf("FileSeek: Error Seek Set is out of bounds\n");
			return FILE_FAIL;
		}
		fds[handle].pos = num_bytes;
	}

	//SEEK CUR
	if (from_where == FILE_SEEK_CUR) {
		if ((fds[hanle].pos + num_bytes > filesize) || (fds[handle].pos + num_bytes < 0)) {
			printf("FileSeek: Error Seek Cur is out of bounds\n");
			return FILE_FAIL;
		}
		fds[handle].pos += num_bytes;
	}

	//SEEK END num_bytes should be negative
	if (from_where == FILE_SEEK_END) {
		if ((num_bytes + filesize < 0) || (num_bytes > 0)) {
			printf("FileSeek: Error Seek Cur is out of bounds\n");
			return FILE_FAIL;
		}
		fds[handle].pos = filesize + num_bytes;
	}

	return FILE_SUCCESS;
}

//delete the file specified by filename. Return FILE_FAIL on failure, and FILE_SUCCESS on success
int FileDelete(char *filename) {
	uint32 inode_handle;
	if ((inode_handle = DfsInodeFilenameExists(filename)) == DFS_FAIL) {
		printf("FileDelete: Error could not delete filename because it doesn't exist\n");
		return FILE_FAIL;
	}

	if ((DfsInodeDelete(inode_handle)) == DFS_FAIL) {
		printf("FileDelete: Error could not delete filename in DfsInodeDelete\n");
		return FILE_FAIL;
	}
	return FILE_SUCCESS;
}