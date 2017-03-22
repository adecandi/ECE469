#include "usertraps.h"
#include "misc.h"

#include "radeon.h"

void main (int argc, char *argv[]) 
{
	sem_t s_procs_completed;	// Semaphore to signal the original process that we're done
	mbox_t mbox_s2, mbox_s;
	char msg[5];
	int rcv;

	s_procs_completed = dstrtol(argv[3], NULL, 10); 	// The "10" means base 10
	mbox_s2 = dstrtol(argv[1], NULL, 10);
	mbox_s = dstrtol(argv[2], NULL, 10);

	if(mbox_open(mbox_s2) != MBOX_SUCCESS) {
		Printf("Split S2 (%d): could not open mbox\n", getpid());
		Exit();
	}

	// Receive 1 S2
	rcv = mbox_recv(mbox_s2, 2, (char *) &msg);
	if(rcv != 2) {
		Printf("Split S2 (%d): could not rcv s2 mbox. Rcv=%d\n", getpid(), rcv);
		Printf(msg); Printf("\n");
		Exit();
	}

	if(mbox_close(mbox_s2) != MBOX_SUCCESS) {
		Printf("Split S2 (%d): could not close mbox\n", getpid());
		Exit();
	}

	if(mbox_open(mbox_s) != MBOX_SUCCESS) {
		Printf("Split S2 (%d): could not open mbox\n", getpid());
		Exit();
	}
	// Send 2 S's
	if(mbox_send(mbox_s, 1, (void *) "S") != MBOX_SUCCESS) {
		Printf("Split S2 (%d): could not send \n", getpid());
		Exit();
	}
	Printf("PID: %d Created a S molecule.\n", getpid());
	if(mbox_send(mbox_s, 1, (void *) "S") != MBOX_SUCCESS) {
		Printf("Split S2 (%d): could not send \n", getpid());
		Exit();
	}	
	Printf("PID: %d Created a S molecule.\n", getpid());

	// Close the mailboxes

	if(mbox_close(mbox_s) != MBOX_SUCCESS) {
		Printf("Split S2 (%d): could not close mbox\n", getpid());
		Exit();
	}

	//Printf("Splitting CO done: PID %d is complete.\n", getpid());
	if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
		Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
		Exit();
	}

}