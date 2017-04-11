#include "usertraps.h"
#include "misc.h"

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

  memory_location = 0x8765430;
  Printf("part2 (%d): Accessing Memory Location: %d (decimal)\n", getpid(), memory_location);
  Printf("part2 (%d): DONE!\n", getpid());

}
