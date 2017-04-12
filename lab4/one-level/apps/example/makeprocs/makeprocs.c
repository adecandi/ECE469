#include "usertraps.h"
#include "misc.h"

#define part1 "part1.dlx.obj"
#define part2 "part2.dlx.obj"
#define part3 "part3.dlx.obj"
#define part4 "part4.dlx.obj"
#define part5 "part5.dlx.obj"
#define part6 "part6.dlx.obj"

void main (int argc, char *argv[])
{
  int num_procs = 0;
  int proc_sel = 0;                    // Used to store number of processes to create
  int i;                               // Loop index variable
  sem_t s_procs_completed;             // Semaphore used to wait until all spawned processes have completed
  char s_procs_completed_str[10];      // Used as command-line argument to pass page_mapped handle to new processes

  if (argc != 2) {
    Printf("Usage: %s <test to run (0) for all>\n", argv[0]);
    Exit();
  }

  Printf("Checking num_procs\n");
  // Convert string from ascii command line argument to integer number
  proc_sel = dstrtol(argv[1], NULL, 10); // the "10" means base 10

  // Switch case to know how many num_procs for the semaphore
  switch(proc_sel) {
    case 0: num_procs = 134;  break;
    case 1: num_procs = 1;    break;
    case 2: num_procs = 1;    break;
    case 3: num_procs = 1;    break;
    case 4: num_procs = 1;    break;
    case 5: num_procs = 100;  break;
    case 6: num_procs = 30;   break;
  }

  // Create semaphore to not exit this process until all other processes 
  // have signalled that they are complete.
  if ((s_procs_completed = sem_create(-(num_procs-1))) == SYNC_FAIL) {
    Printf("makeprocs (%d): Bad sem_create\n", getpid());
    Exit();
  }

  // Setup the command-line arguments for the new processes.  We're going to
  // pass the handles to the semaphore as strings
  // on the command line, so we must first convert them from ints to strings.
  ditoa(s_procs_completed, s_procs_completed_str);

  if(proc_sel == 1 || proc_sel == 0) {
    Printf("-------------------------------------------------------------------------------------\n");
    Printf("makeprocs (%d): part1\n", getpid());
    Printf("makeprocs (%d): Creating hello world\n", getpid());
    process_create(part1, s_procs_completed_str, NULL);
  } 

  if(proc_sel == 2 || proc_sel == 0) {
    Printf("-------------------------------------------------------------------------------------\n");
    Printf("makeprocs (%d): part2\n", getpid());
    Printf("makeprocs (%d): Creating process to test access beyond max\n", getpid());
    process_create(part2, s_procs_completed_str, NULL);
  }

  if(proc_sel == 3 || proc_sel == 0) {
    Printf("-------------------------------------------------------------------------------------\n");
    Printf("makeprocs (%d): part3\n", getpid());
    Printf("makeprocs (%d): Creating process to test unallocated\n", getpid());
    process_create(part3, s_procs_completed_str, NULL);
  } 

  if(proc_sel == 4 || proc_sel == 0) {
    Printf("-------------------------------------------------------------------------------------\n");
    Printf("makeprocs (%d): part3\n", getpid());
    Printf("makeprocs (%d): Creating process to test unallocated\n", getpid());
    process_create(part4, s_procs_completed_str, NULL);
  } 

  if(proc_sel == 5 || proc_sel == 0) {
    Printf("-------------------------------------------------------------------------------------\n");
    Printf("makeprocs (%d): part5\n", getpid());
    Printf("makeprocs (%d): Creating 100 hello worlds\n", getpid());
    for(i = 0; i < 100; i++) {
      process_create(part5, s_procs_completed_str, NULL);
    }
  }

  if(proc_sel == 6 || proc_sel == 0) {
    Printf("-------------------------------------------------------------------------------------\n");
    Printf("makeprocs (%d): part6\n", getpid());
    Printf("makeprocs (%d): Creating 30 processes looping to high numbers\n", getpid());
    for(i = 0; i < 30; i++) {
      process_create(part6, s_procs_completed_str, NULL);

    }
  }     

  if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in %s\n", s_procs_completed, argv[0]);
    Exit();
  }

  Printf("-------------------------------------------------------------------------------------\n");
  Printf("makeprocs (%d): All other processes completed, exiting main process.\n", getpid());

}
