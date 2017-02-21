#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "radeon.h"

void main (int argc, char *argv[]) 
{
	sem_t s_procs_completed;	// Semaphore to signal the original process that we're done
	sem_t s_s2;					// Semaphore for s2
	sem_t s_s;					// Semaphore for s
	
	int num_s;					// Number of s2 coming from makeprocs

	s_procs_completed = dstrtol(argv[1], NULL, 10); 	// The "10" means base 10
	s_s2 = dstrtol(argv[2], NULL, 10);					// sem for s2
	s_s = dstrtol(argv[3], NULL, 10);
	num_s = dstrtol(argv[4], NULL, 10);				// number of s2

	while(num_s > 0) {
		if(sem_wait(s_s2) != SYNC_SUCCESS) {
			Printf("Bad semaphore in %d", s_s2);
			Exit();
		}
		if(sem_signal(s_s) != SYNC_SUCCESS) {
			Printf("Bad semaphore in %d", s_s);
			Exit();
		}
		if(sem_signal(s_s) != SYNC_SUCCESS) {
			Printf("Bad semaphore in %d", s_s);
			Exit();
		}
		num_s--;
		num_s--;
		Printf("PID: %d Created a S molecules.\n", getpid());
		Printf("PID: %d Created a S molecules.\n", getpid());
	}

	Printf("Splitting S2 done: PID %d is complete.\n", getpid());
	if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
		Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
		Exit();
	}
}