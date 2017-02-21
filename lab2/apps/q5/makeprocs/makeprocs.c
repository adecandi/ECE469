#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "radeon.h"

void main (int argc, char *argv[])
{
	int s2_exp, co_exp, s_exp, o2_exp, so4_exp;
	sem_t s_procs_completed;        // Semaphore used to wait until all spawned processes have completed
	int numprocs = 5;               // Used to store number of processes to create

	sem_t s_s2;
	sem_t s_s;
	sem_t s_co;
	sem_t s_o2;
	sem_t s_so4;

	char s_procs_completed_str[10]; // Used as command-line argument to pass page_mapped handle to new processes
	char s_s2_str[10];
	char s_s_str[10];
	char s_co_str[10];
	char s_o2_str[10];
	char s_so4_str[10];

	char s2_exp_str[10];
	char co_exp_str[10];
	char s_exp_str[10];
	char o2_exp_str[10];
	char so4_exp_str[10];

	if (argc != 3) {
    	Printf("Usage: "); Printf(argv[0]); Printf(" <number of S2> <number of CO>\n");
    	Exit();
  	}

  	s2_exp = dstrtol(argv[1], NULL, 10);
  	co_exp = dstrtol(argv[2], NULL, 10);
  	s_exp = s2_exp * 2;
  	o2_exp = co_exp / 2;
  	so4_exp = min(s_exp, o2_exp / 2);

	// Create semaphore to not exit this process until all other processes 
	// have signalled that they are complete.  To do this, we will initialize
	// the semaphore to (-1) * (number of signals), where "number of signals"
	// should be equal to the number of processes we're spawning - 1.  Once 
	// each of the processes has signaled, the semaphore should be back to
	// zero and the final sem_wait below will return.
  	if ((s_procs_completed = sem_create(-(numprocs-1))) == SYNC_FAIL) {
    	Printf("Bad sem_create in "); Printf(argv[0]); Printf("\n");
    	Exit();
  	}
  	if (s_s2 = sem_create(0) == SYNC_FAIL) {
  		Printf("Bad sem_create in s_s2\n");
    	Exit();
  	}
  	if (s_s = sem_create(0) == SYNC_FAIL) {
  		Printf("Bad sem_create in s_s\n");
    	Exit();
  	}
  	if (s_co = sem_create(0) == SYNC_FAIL) {
  		Printf("Bad sem_create in s_co\n");
    	Exit();
  	}
	if (s_o2 = sem_create(0) == SYNC_FAIL) {
  		Printf("Bad sem_create in s_o2\n");
    	Exit();
  	}
  	if (s_so4 = sem_create(0) == SYNC_FAIL) {
  		Printf("Bad sem_create in s_so4\n");
    	Exit();
  	}

  	ditoa(s_s2, s_s2_str);
  	ditoa(s_s, s_s_str);
  	ditoa(s_o2, s_o2_str);
  	ditoa(s_co, s_co_str);
  	ditoa(s_o2, s_o2_str);
  	ditoa(s_so4, s_so4_str);
  	ditoa(s_procs_completed, s_procs_completed_str);

  	ditoa(s2_exp, s2_exp_str);
  	ditoa(s_exp, s_exp_str);
  	ditoa(o2_exp, o2_exp_str);
  	ditoa(co_exp, co_exp_str);
  	ditoa(so4_exp, so4_exp_str);

  	process_create(INJECT_S2, s_procs_completed_str, s_s2_str, s2_exp_str, NULL);
  	process_create(INJECT_CO, s_procs_completed_str, s_co_str, co_exp_str, NULL);
  	process_create(SPLIT_S2, s_procs_completed_str, s_s2_str, s_s_str, s_exp_str, NULL);
  	process_create(SPLIT_CO, s_procs_completed_str, s_co_str, s_o2_str, o2_exp_str, NULL);
  	process_create(MAKE_SO4, s_procs_completed_str, s_s_str, s_o2_str, s_so4_str, so4_exp_str, NULL);

	// And finally, wait until all spawned processes have finished.
	if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
		Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf("\n");
		Exit();
	}
	Printf("All other processes completed, exiting main process.\n");
}