#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "radeon.h"

void main (int argc, char *argv[]) 
{
	sem_t s_procs_completed;	// Semaphore to signal the original process that we're done
	sem_t s_co;					// Semaphore for co
	sem_t s_o2;					// Semaphore for o2

	int num_o2;					// Number of o2 coming from makeprocs

	s_procs_completed = dstrtol(argv[1], NULL, 10); 	// The "10" means base 10
	s_co = dstrtol(argv[2], NULL, 10);					// sem for co
	s_o2 = dstrtol(argv[3], NULL, 10);					// sem for o2
	num_o2 = dstrtol(argv[4], NULL, 10);				// number of co

	while(num_o2 > 0) {
		if(sem_wait(s_co) != SYNC_SUCCESS) {
			Printf("Bad semaphore in %d", s_co);
			Exit();
		}
		if(sem_wait(s_co) != SYNC_SUCCESS) {
			Printf("Bad semaphore in %d", s_co);
			Exit();
		}
		if(sem_wait(s_co) != SYNC_SUCCESS) {
			Printf("Bad semaphore in %d", s_co);
			Exit();
		}
		if(sem_wait(s_co) != SYNC_SUCCESS) {
			Printf("Bad semaphore in %d", s_co);
			Exit();
		}
		if(sem_signal(s_o2) != SYNC_SUCCESS) {
			Printf("Bad semaphore in %d", s_o2);
			Exit();
		}
		if(sem_signal(s_o2) != SYNC_SUCCESS) {
			Printf("Bad semaphore in %d", s_o2);
			Exit();
		}
		num_o2--;
		num_o2--;
		Printf("PID: %d Created O2 molecule.\n", getpid());
		Printf("PID: %d Created O2 molecule.\n", getpid());
		Printf("PID: %d Created C2 molecule.\n", getpid());
		Printf("PID: %d Created C2 molecule.\n", getpid());
	}

	Printf("Splitting CO done: PID %d is complete.\n", getpid());
	if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
		Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
		Exit();
	}
}