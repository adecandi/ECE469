#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "dfs.h"
#include "files.h"
#include "synch.h"

// You have already been told about the most likely places where you should use locks. You may use 
// additional locks if it is really necessary.

// STUDENT: put your file-level functions here

void FileModuleInit() {
	return -1;
}
int FileOpen(char *filename, char *mode) {
	return -1;
}
int FileClose(int handle) {
	return -1;
}
int FileRead(int handle, void *mem, int num_bytes) {
	return -1;
}
int FileWrite(int handle, void *mem, int num_bytes) {
	return -1;
}
int FileSeek(int handle, int num_bytes, int from_where) {
	return -1;
}
int FileDelete(char *filename) {
	return -1;
}