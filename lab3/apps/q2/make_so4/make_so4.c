#include "usertraps.h"
#include "misc.h"

#include "radeon.h"

void main (int argc, char *argv[]) 
{
	sem_t s_procs_completed;	// Semaphore to signal the original process that we're done
	mbox_t mbox_s, mbox_o2, mbox_so4;
	char s_s[1], s_o2[2];

	int test;

	s_procs_completed = dstrtol(argv[4], NULL, 10); 	// The "10" means base 10
	mbox_so4 = dstrtol(argv[1], NULL, 10);
	mbox_s = dstrtol(argv[2], NULL, 10);
	mbox_o2 = dstrtol(argv[3], NULL, 10);

	if(mbox_open(mbox_so4) != MBOX_SUCCESS) {
		Printf("Make SO4 (%d): could not open mbox\n", getpid());
		Exit();
	}

	if(mbox_open(mbox_s) != MBOX_SUCCESS) {
		Printf("Make SO4 (%d): could not open mbox\n", getpid());
		Exit();
	}

	if(mbox_open(mbox_o2) != MBOX_SUCCESS) {
		Printf("Make SO4 (%d): could not open mbox\n", getpid());
		Exit();
	}

	// Receive 
	test = mbox_recv(mbox_o2, 2, (char *) &s_o2);
	if(test != 2) {
		Printf("Make SO4 (%d): could not rcv o2 mbox 1\n", getpid());
		Printf("test=%d\n", test); Printf(s_o2); Printf("\n");
		Exit();
	}
	if(mbox_recv(mbox_s, 2, (char *) &s_s) != 1) {
		Printf("Make SO4 (%d): could not rcv s mbox\n", getpid());
		Exit();
	}
	if(mbox_recv(mbox_o2, 2, (char *) &s_o2) != 2) {
		Printf("Make SO4 (%d): could not rcv o2 mbox 2\n", getpid());
		Exit();
	}

	// Send SO4
	if(mbox_send(mbox_so4, 3, (void *) "SO4") != MBOX_SUCCESS) {
		Printf("Make SO4 (%d): could not send \n", getpid());
		Exit();
	}
	Printf("PID: %d Created a SO4 molecule.\n", getpid());
	// Close the mailboxes
	if(mbox_close(mbox_so4) != MBOX_SUCCESS) {
		Printf("Make SO4 (%d): could not close mbox\n", getpid());
		Exit();
	}

	if(mbox_close(mbox_s) != MBOX_SUCCESS) {
		Printf("Make SO4 (%d): could not close mbox\n", getpid());
		Exit();
	}

	if(mbox_close(mbox_o2) != MBOX_SUCCESS) {
		Printf("Make SO4 (%d): could not close mbox\n", getpid());
		Exit();
	}

	//Printf("Creating SO4 done: PID %d is complete.\n", getpid());
	if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
		Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
		Exit();
	}
}