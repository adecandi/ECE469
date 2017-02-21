#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "radeon.h"

void main (int argc, char *argv[]) 
{
	sem_t s_procs_completed;	// Semaphore to signal the original process that we're done
	sem_t s_co;					// Semaphore for co

	int num_co;					// Number of co coming from makeprocs

	s_procs_completed = dstrtol(argv[1], NULL, 10); 	// The "10" means base 10
	s_co = dstrtol(argv[2], NULL, 10);					// sem for co
	num_co = dstrtol(argv[3], NULL, 10);				// number of co

	while(num_co > 0) {
		if(sem_signal(s_co) != SYNC_SUCCESS) {
			Printf("Bad semaphore in %d", s_co);
			Exit();
		}
		num_co--;
		Printf("PID: %d Created a CO molecule.\n", getpid());
	}

	//Printf("Injecting CO: PID %d is complete.\n", getpid());
	if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
		Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
		Exit();
	}
}