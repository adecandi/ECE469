#include "usertraps.h"
#include "misc.h"

#include "radeon.h"

void main (int argc, char *argv[]) 
{
	sem_t s_procs_completed;	// Semaphore to signal the original process that we're done

	mbox_t mbox_s2;

	s_procs_completed = dstrtol(argv[2], NULL, 10); 	// The "10" means base 10
	mbox_s2 = dstrtol(argv[1], NULL, 10);

	if(mbox_open(mbox_s2) != MBOX_SUCCESS) {
		Printf("Injection S2 (%d): could not open mbox\n", getpid());
		Exit();
	}

	// Inject 1 S2
	if(mbox_send(mbox_s2, 2, (void *)"S2") != MBOX_SUCCESS) {
		Printf("Injection S2 (%d): could not send\n", getpid());
		Exit();
	}

	Printf("PID: %d Created a S2 molecule.\n", getpid());

	if(mbox_close(mbox_s2) != MBOX_SUCCESS) {
		Printf("Injection S2 (%d): could not close mbox\n", getpid());
		Exit();
	}

	//Printf("Splitting S2 done: PID %d is complete.\n", getpid());
	if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
		Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
		Exit();
	}
}