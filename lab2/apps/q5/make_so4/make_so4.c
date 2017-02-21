#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "radeon.h"

void main (int argc, char *argv[]) 
{
	sem_t s_procs_completed;	// Semaphore to signal the original process that we're done
	sem_t s_s;					// Semaphore for s
	sem_t s_o2;					// Semaphore for o2
	sem_t s_so4;				// Semaphore for so4
	
	int num_so4;				// Number of s2 coming from makeprocs

	s_procs_completed = dstrtol(argv[1], NULL, 10); 	// The "10" means base 10
	s_s = dstrtol(argv[2], NULL, 10);					// sem for s2
	s_o2 = dstrtol(argv[3], NULL, 10);
	s_so4 = dstrtol(argv[4], NULL, 10);
	num_so4 = dstrtol(argv[5], NULL, 10);				// number of s2

	while(num_so4 > 0) {
		// Consume 1 s
		if(sem_wait(s_s) != SYNC_SUCCESS) {
			Printf("Bad semaphore in %d", s_s);
			Exit();
		}

		// Consume 2 o2
		if(sem_wait(s_o2) != SYNC_SUCCESS) {
			Printf("Bad semaphore in %d", s_o2);
			Exit();
		}
		if(sem_wait(s_o2) != SYNC_SUCCESS) {
			Printf("Bad semaphore in %d", s_o2);
			Exit();
		}

		// Make 1 so4
		if(sem_signal(s_so4) != SYNC_SUCCESS) {
			Printf("Bad semaphore in %d", s_so4);
			Exit();
		}

		num_so4--;
		Printf("PID: %d Created a SO4 molecule.\n", getpid());
	}

	Printf("Creating SO4 done: PID %d is complete.\n", getpid());
	if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
		Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
		Exit();
	}
}