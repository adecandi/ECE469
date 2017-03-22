#include "usertraps.h"
#include "misc.h"

#include "radeon.h"

void main (int argc, char *argv[]) 
{
	sem_t s_procs_completed;	// Semaphore to signal the original process that we're done

	mbox_t mbox_co;

	s_procs_completed = dstrtol(argv[2], NULL, 10); 	// The "10" means base 10
	mbox_co = dstrtol(argv[1], NULL, 10);

	if(mbox_open(mbox_co) != MBOX_SUCCESS) {
		Printf("Injection CO (%d): could not open mbox\n", getpid());
		Exit();
	}

	// Inject 1 CO
	if(mbox_send(mbox_co, 2, (void *)"CO") != MBOX_SUCCESS) {
		Printf("Injection CO (%d): could not send\n", getpid());
		Exit();
	}

	Printf("PID: %d Created a CO molecule.\n", getpid());

	if(mbox_close(mbox_co) != MBOX_SUCCESS) {
		Printf("Injection CO (%d): could not close mbox\n", getpid());
		Exit();
	}

	//Printf("Splitting CO done: PID %d is complete.\n", getpid());
	if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
		Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
		Exit();
	}
}