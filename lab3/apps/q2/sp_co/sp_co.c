#include "usertraps.h"
#include "misc.h"

#include "radeon.h"

void main (int argc, char *argv[]) 
{
	sem_t s_procs_completed;	// Semaphore to signal the original process that we're done
	mbox_t mbox_co, mbox_o2;
	char msg[2];
	int rcv1, rcv2, rcv3, rcv4;

	s_procs_completed = dstrtol(argv[3], NULL, 10); 	// The "10" means base 10
	mbox_co = dstrtol(argv[1], NULL, 10);
	mbox_o2 = dstrtol(argv[2], NULL, 10);

	if(mbox_open(mbox_co) != MBOX_SUCCESS) {
		Printf("Split CO (%d): could not open mbox\n", getpid());
		Exit();
	}

	// RECIEVE FOUR CO's
	rcv1 = mbox_recv(mbox_co, 2, (void *) &msg);
	if(rcv1 != 2) {
		Printf("Split CO (%d): could not rcv co mbox 1. Rcv=%d\n", getpid(), rcv1);
		Exit();
	}
	rcv2 = mbox_recv(mbox_co, 2, (void *) &msg);
	if(rcv2 != 2) {
		Printf("Split CO (%d): could not rcv co mbox 2. Rcv=%d\n", getpid(), rcv2);
		Exit();
	}
	rcv3 = mbox_recv(mbox_co, 2, (void *) &msg);
	if(rcv3 != 2) {
		Printf("Split CO (%d): could not rcv co mbox 3. Rcv=%d\n", getpid(), rcv3);
		Printf((char *) msg); Printf("\n");
		Exit();
	}
	rcv4 = mbox_recv(mbox_co, 2, (void *) &msg);
	if(rcv4 != 2) {
		Printf("Split CO (%d): could not rcv co mbox 4. Rcv=%d\n", getpid(), rcv4);
		Exit();
	}
	if(mbox_close(mbox_co) != MBOX_SUCCESS) {
		Printf("Split CO (%d): could not close mbox\n", getpid());
		Exit();
	}
	
	if(mbox_open(mbox_o2) != MBOX_SUCCESS) {
		Printf("Split CO (%d): could not open mbox\n", getpid());
		Exit();
	}

	// CREATE 2 O2's
	if(mbox_send(mbox_o2, 2, (void *) "O2") != MBOX_SUCCESS) {
		Printf("Split CO (%d): could not send \n", getpid());
		Exit();
	}
	Printf("PID: %d Created an O2 molecule.\n", getpid());
	if(mbox_send(mbox_o2, 2, (void *) "O2") != MBOX_SUCCESS) {
		Printf("Split CO (%d): could not send \n", getpid());
		Exit();
	}	
	Printf("PID: %d Created an O2 molecule.\n", getpid());

	// Close mailboxes
	
	if(mbox_close(mbox_o2) != MBOX_SUCCESS) {
		Printf("Split CO (%d): could not close mbox\n", getpid());
		Exit();
	}

	//Printf("Splitting CO done: PID %d is complete.\n", getpid());
	if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
		Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
		Exit();
	}
}