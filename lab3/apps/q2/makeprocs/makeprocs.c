#include "usertraps.h"
#include "misc.h"

#include "radeon.h"

void main (int argc, char *argv[])
{
	int s2_exp, co_exp, s_exp, o2_exp, so4_exp;
	int s_reac, o2_reac, so4_reac;
	int numprocs, i;
	sem_t s_procs_completed;        // Semaphore used to wait until all spawned processes have completed

	mbox_t mbox_s2, mbox_s, mbox_co, mbox_o2, mbox_so4;

	char s_procs_completed_str[10]; // Used as command-line argument to pass page_mapped handle to new processes
	char s2_str[10], s_str[10], co_str[10], o2_str[10], so4_str[10];


	if (argc != 3) {
		Printf("Usage: "); Printf(argv[0]); Printf(" <number of S2> <number of CO>\n");
		Exit();
	}

	s2_exp = dstrtol(argv[1], NULL, 10);
	co_exp = dstrtol(argv[2], NULL, 10);
	s_exp = s2_exp * 2;
	s_reac = s2_exp;
	o2_exp = (co_exp >= 4) ? (co_exp / 2) : 0;
	o2_reac = co_exp / 4;
	so4_exp = (s_exp > (o2_exp / 2)) ? (o2_exp / 2) : s_exp;
	numprocs = s2_exp + co_exp + s_reac + o2_reac + so4_exp;

	// Create semaphore to not exit this process until all other processes 
	// have signalled that they are complete.  To do this, we will initialize
	// the semaphore to (-1) * (number of signals), where "number of signals"
	// should be equal to the number of processes we're spawning - 1.  Once 
	// each of the processes has signaled, the semaphore should be back to
	// zero and the final sem_wait below will return.
	if ((s_procs_completed = sem_create(-(numprocs-1))) == SYNC_FAIL) {
		Printf("Bad sem_create in "); Printf(argv[0]); Printf("\n");
		Exit();
	}
	if (mbox_s2 = mbox_create() == MBOX_FAIL) {
		Printf("Bad mbox_create in s2\n");
		Exit();
	}
	if (mbox_co = mbox_create() == MBOX_FAIL) {
		Printf("Bad mbox_create in co\n");
		Exit();
	}
	if (mbox_s = mbox_create() == MBOX_FAIL) {
		Printf("Bad mbox_create in s\n");
		Exit();
	}
	if (mbox_o2 = mbox_create() == MBOX_FAIL) {
		Printf("Bad mbox_create in o2\n");
		Exit();
	}
	if (mbox_so4 = mbox_create() == MBOX_FAIL) {
		Printf("Bad mbox_create in so4\n");
		Exit();
	}

	if (mbox_open(mbox_s2) != MBOX_SUCCESS) {
		Printf("Could not open mailbox for s2\n");
		Exit();
	}
	if (mbox_open(mbox_co) != MBOX_SUCCESS) {
		Printf("Could not open mailbox for co\n");
		Exit();
	}
	if (mbox_open(mbox_s) != MBOX_SUCCESS) {
		Printf("Could not open mailbox for s\n");
		Exit();
	}
	if (mbox_open(mbox_o2) != MBOX_SUCCESS) {
		Printf("Could not open mailbox for o2\n");
		Exit();
	}
	if (mbox_open(mbox_so4) != MBOX_SUCCESS) {
		Printf("Could not open mailbox for so4\n");
		Exit();
	}

	ditoa(mbox_s2, s2_str);
	ditoa(mbox_s, s_str);
	ditoa(mbox_o2, o2_str);
	ditoa(mbox_co, co_str);
	ditoa(mbox_so4, so4_str);
	ditoa(s_procs_completed, s_procs_completed_str);

	for(i = 0; i < numprocs; i++) {
		if (i < s2_exp)
			process_create(INJECT_S2, 0, 0, s2_str, s_procs_completed_str, NULL);
		if (i < co_exp)
			process_create(INJECT_CO, 0, 0, co_str, s_procs_completed_str, NULL);
	}
	for(i = 0; i < numprocs; i++) {
		if (i < s_reac)
			process_create(SPLIT_S2, 0, 0, s2_str, s_str, s_procs_completed_str, NULL);
		if (i < o2_reac)
			process_create(SPLIT_CO, 0, 0, co_str, o2_str, s_procs_completed_str, NULL);
	}
	for(i = 0; i < numprocs; i++) {	
		if (i < so4_exp)
			process_create(MAKE_SO4, 0, 0, so4_str, s_str, o2_str, s_procs_completed_str, NULL);
	}

	// And finally, wait until all spawned processes have finished.
	if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
		Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf("\n");
		Exit();
	}

	if (mbox_close(mbox_s2) != SYNC_SUCCESS) {
		Printf("Could not close mailbox for s2\n");
		Exit();
	}
	if (mbox_close(mbox_s) != SYNC_SUCCESS) {
		Printf("Could not close mailbox for s\n");
		Exit();
	}
	if (mbox_close(mbox_co) != SYNC_SUCCESS) {
		Printf("Could not close mailbox for co\n");
		Exit();
	}
	if (mbox_close(mbox_o2) != SYNC_SUCCESS) {
		Printf("Could not close mailbox for o2\n");
		Exit();
	}
	if (mbox_close(mbox_so4) != SYNC_SUCCESS) {
		Printf("Could not close mailbox for so4\n");
		Exit();
	}

	Printf("All other processes completed, exiting main process.\n");
}