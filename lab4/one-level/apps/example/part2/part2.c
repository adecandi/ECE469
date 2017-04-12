#include "usertraps.h"
#include "misc.h"
#include "os/memory_constants.h"

void main (int argc, char *argv[])
{
  sem_t s_procs_completed; // Semaphore to signal the original process that we're done
  int * mem_loc;  //memory location

  if (argc != 2) { 
    Printf("Usage: %s <handle_to_procs_completed_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  s_procs_completed = dstrtol(argv[1], NULL, 10);

  // Now print a message to show that everything worked
  Printf("part2 (%d): Testing out of bounds virtual access\n", getpid());
  Printf("part2 (%d): START!\n", getpid());


  // We signal the semaphore before the process is finished because this 
  // process will die
  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("part2 (%d): Bad semaphore s_procs_completed (%d)!\n", getpid(), s_procs_completed);
    Exit();
  }
  // set the memory location to greater than the max
  mem_loc = (MEM_MAX_VIRTUAL_ADDRESS + 1);
  // print where we will test
  Printf("part2 (%d): Accessing Memory Location: %d (decimal)\n", getpid(), mem_loc);
  // attempt to access that location
  Printf("part2 (%d): Accessing Memory Location: %d (decimal)\n", getpid(), *mem_loc);
  Printf("part2 (%d): DONE!\n", getpid());

}
