#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "radeon.h"

void main (int argc, char *argv[]) 
{
	sem_t s_procs_completed;	// Semaphore to signal the original process that we're done
	sem_t s_s2;					// Semaphore for s2

	int num_s2;					// Number of s2 coming from makeprocs

	s_procs_completed = dstrtol(argv[1], NULL, 10); 	// The "10" means base 10
	s_s2 = dstrtol(argv[2], NULL, 10);					// sem for s2
	num_s2 = dstrtol(argv[3], NULL, 10);				// number of s2

	while(num_s2 > 0) {
		if(SemSignal(s_s2) != SYNC_SUCCESS) {
			Printf("Bad semaphore in %d", s_s2);
			Exit();
		}
		num_s2--;
		Printf("PID: %d Created a S2 molecule.", getpid());
	}

	Printf("Injecting S2: PID %d is complete.\n", getpid());
	if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
		Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
		Exit();
	}
}