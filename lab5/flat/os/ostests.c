#include "ostraps.h"
#include "dlxos.h"
#include "traps.h"
#include "disk.h"
#include "dfs.h"
#include "misc.h"

void RunOSTests() {
	int fhandle;
	char mellow[] = "mellow";
	char yellow[] = "yellow";
	char jello[] = "jello";
	char fellow[] = "fellow";

  char buffer[512];

  printf("Starting RunOSTests function\n");

	if(DfsInodeFilenameExists("hello") == DFS_FAIL) {
		printf("\n================= Start ostests (first run)\n");
		printf("--- Initializing hello file for testing\n");
		printf("--- run this program again to see persistence behaviour\n");
  	printf("- Hello file did not exist\n");
  	printf("+ Creating hello file\n");
  	fhandle = DfsInodeOpen("hello");
  	
  	printf("**** Writing mellow on hello file @ random address 1240\n");
	  DfsInodeWriteBytes(fhandle, mellow, 1240, dstrlen(mellow) + 1);
	  printf("**** Call DfsInodeFileSize function: returned %d\n", DfsInodeFilesize(fhandle));

	  printf("**** Writing yellow on hello file @ random address 1250\n");
	  DfsInodeWriteBytes(fhandle, yellow, 1250, dstrlen(yellow) + 1);
	  printf("**** Call DfsInodeFileSize function: returned %d\n", DfsInodeFilesize(fhandle));

	  printf("---- In 512 DFS block size; 5160 byte will be in indirect block addressed\n");
	  printf("**** Writing fellow on hello file @ probble indirected addressed (5120 + 40) \n");
	  DfsInodeWriteBytes(fhandle, fellow, 5160, dstrlen(fellow) + 1);
	  printf("**** Call DfsInodeFileSize function: returned %d\n", DfsInodeFilesize(fhandle));
		printf("================= End ostests\n\n");
	  return;
  }
  else {
		printf("\n================= Start ostests (second run)\n");
  	printf("+ Hello file did exist\n");
  	printf("++ Reopening hello file\n");
  	fhandle = DfsInodeOpen("hello");	
  }
  if(DfsInodeFilenameExists("hello") == fhandle) {
  	printf("++ Researched for hello and found same hello file, filehandle:%d\n", fhandle);
  }

  printf("***** Check @ 1240 into buffer (should be mellow)\n");
  DfsInodeReadBytes(fhandle, buffer, 1240, dstrlen(mellow) + 1);
  printf("***** Found: %s\n", buffer);

  printf("***** Check @ 1250 into buffer (should be yellow)\n");
  DfsInodeReadBytes(fhandle, buffer, 1250, dstrlen(yellow) + 1);
  printf("***** Found: %s\n", buffer);

  printf("++ Re-Writing jello on hello file @ random address 1240\n");
  printf("++ tests whether writes do a read on non algined writes\n");
	DfsInodeWriteBytes(fhandle, jello, 1240, dstrlen(jello) + 1);
	printf("++ Call DfsInodeFileSize function: returned %d\n", DfsInodeFilesize(fhandle));

  printf("***** Check @ 1250 into buffer (should be yellow)\n");
  DfsInodeReadBytes(fhandle, buffer, 1250, dstrlen(yellow) + 1);
  printf("***** Found: %s\n", buffer);

  printf("***** Check @ 1240 into buffer (should be jello)\n");
  DfsInodeReadBytes(fhandle, buffer, 1240, dstrlen(jello) + 1);
  printf("***** Found: %s\n", buffer);

  printf("---- In 512 DFS block size; 5160 byte will be in indirect block addressed\n");
  printf("***** Check @ 5160 into buffer (should be fellow)\n");
  DfsInodeReadBytes(fhandle, buffer, 5160, dstrlen(fellow) + 1);
  printf("***** Found: %s\n", buffer);

  printf("---- Deleting hello file\n");
  DfsInodeDelete(fhandle);
	printf("================= End ostests\n\n");
}
