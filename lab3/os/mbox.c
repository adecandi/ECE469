#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "synch.h"
#include "queue.h"
#include "mbox.h"

static mbox mbox_structs[MBOX_NUM_MBOXES];
static mbox_message mbox_mess_structs[MBOX_NUM_BUFFERS];

//-------------------------------------------------------
//
// void MboxModuleInit();
//
// Initialize all mailboxes.  This process does not need
// to worry about synchronization as it is called at boot
// time.  Only initialize necessary items here: you can
// initialize others in MboxCreate.  In other words, 
// don't waste system resources like locks and semaphores
// on unused mailboxes.
//
//-------------------------------------------------------

void MboxModuleInit() {
	int i, j;
	for(i = 0; i < MBOX_NUM_MBOXES; i++) {
		mbox_structs[i].inuse = 0;
		mbox_structs[i].used = 0;
		mbox_structs[i].count = 0;
		for(j = 0; j < PROCESS_MAX_PROCS; j++) {
			mbox_structs[i].pids[j] = 0;
		}
	}

}

//-------------------------------------------------------
//
// mbox_t MboxCreate();
//
// Allocate an available mailbox structure for use. 
//
// Returns the mailbox handle on success
// Returns MBOX_FAIL on error.
//
//-------------------------------------------------------
mbox_t MboxCreate() {
	mbox_t available = 0;
	while(mbox_structs[available].inuse == 1) { 
		available++;
		if(available > MBOX_NUM_MBOXES - 1) {
			return MBOX_FAIL;
		}
	}

	mbox_structs[available].inuse = 1;

	if((mbox_structs[available].l = LockCreate()) == SYNC_FAIL) {
		printf("Bad LockCreate in MboxCreate\n");
    	exitsim();
	}

	if((mbox_structs[available].s_empty = SemCreate(MBOX_MAX_BUFFERS_PER_MBOX-1)) == SYNC_FAIL) {
		printf("Bad SemCreate in MboxCreate\n"); 
		exitsim();
	}

	if((mbox_structs[available].s_full = SemCreate(0)) == SYNC_FAIL) {
		printf("Bad SemCreate in MboxCreate\n");
		exitsim();
	}

	if(AQueueInit(&mbox_structs[available].msg) != QUEUE_SUCCESS) {
		printf("FATAL ERROR: could not initialize message queue in MboxCreate\n");
		exitsim();
	}
  
	return available;
}

//-------------------------------------------------------
// 
// void MboxOpen(mbox_t);
//
// Open the mailbox for use by the current process.  Note
// that it is assumed that the internal lock/mutex handle 
// of the mailbox and the inuse flag will not be changed 
// during execution.  This allows us to get the a valid 
// lock handle without a need for synchronization.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
//-------------------------------------------------------
int MboxOpen(mbox_t handle) {
	if(handle < 0) return MBOX_FAIL;
	if(handle > MBOX_NUM_MBOXES) return MBOX_FAIL;
	if(mbox_structs[handle].inuse == 0) return MBOX_FAIL;



	if(LockHandleAcquire(mbox_structs[handle].l) != SYNC_SUCCESS) {
		printf("Lock unable to be acquired in MboxOpen in %d \n", GetCurrentPid());
		exitsim();
	}

	mbox_structs[handle].used++;
	mbox_structs[handle].pids[GetCurrentPid()] = 1;

	if(LockHandleRelease(mbox_structs[handle].l) != SYNC_SUCCESS) {
		printf("Lock unable to be released in MboxOpen in %d \n", GetCurrentPid());
		exitsim();
	}
	
	return MBOX_SUCCESS;
}

//-------------------------------------------------------
//
// int MboxClose(mbox_t);
//
// Close the mailbox for use to the current process.
// If the number of processes using the given mailbox
// is zero, then disable the mailbox structure and
// return it to the set of available mboxes.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
//-------------------------------------------------------
int MboxClose(mbox_t handle) {
	Link *l;

	if(handle < 0) return MBOX_FAIL;
	if(handle > MBOX_NUM_MBOXES) return MBOX_FAIL;
	if(mbox_structs[handle].inuse == 0) return MBOX_FAIL;

	if(LockHandleAcquire(mbox_structs[handle].l) != SYNC_SUCCESS) {
		printf("Lock unable to be acquired in MboxOpen in %d \n", GetCurrentPid());
		exitsim();
	}

    mbox_structs[handle].used--;
    mbox_structs[handle].pids[GetCurrentPid()] = 0;

    if (mbox_structs[handle].used == 0) {
    	while(!AQueueEmpty(&mbox_structs[handle].msg)) {
    		l = AQueueFirst(&mbox_structs[handle].msg);
    		AQueueRemove(&l);
    	}
    	mbox_structs[handle].inuse = 0;
    }

    if(LockHandleRelease(mbox_structs[handle].l) != SYNC_SUCCESS) {
		printf("Lock unable to be released in MboxOpen in %d \n", GetCurrentPid());
		exitsim();
	}
    return MBOX_SUCCESS;
}

//-------------------------------------------------------
//
// int MboxSend(mbox_t handle,int length, void* message);
//
// Send a message (pointed to by "message") of length
// "length" bytes to the specified mailbox.  Messages of
// length 0 are allowed.  The call 
// blocks when there is not enough space in the mailbox.
// Messages cannot be longer than MBOX_MAX_MESSAGE_LENGTH.
// Note that the calling process must have opened the 
// mailbox via MboxOpen.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
//-------------------------------------------------------
int MboxSend(mbox_t handle, int length, void* message) {
	int i;
	Link *l;
	int cpid = GetCurrentPid();

	if (length <= 0) return MBOX_FAIL;
	if (handle < 0) return MBOX_FAIL;
	if (handle > MBOX_NUM_MBOXES) return MBOX_FAIL;
	if (length > MBOX_MAX_MESSAGE_LENGTH) return MBOX_FAIL;

	if(mbox_structs[handle].pids[cpid] == 0) {
		return MBOX_FAIL;
	}

	if(SemHandleWait(mbox_structs[handle].s_empty) == SYNC_FAIL) {
		printf("Bad sem handle wait in MboxSend\n");
		exitsim();
	}

	if(LockHandleAcquire(mbox_structs[handle].l) != SYNC_SUCCESS) {
		printf("Lock unable to be acquired in MboxSend in %d \n", GetCurrentPid());
		exitsim();
	}

	for(i=0; i < MBOX_NUM_BUFFERS; i++) {
		if(mbox_mess_structs[i].inuse == 0) {
			mbox_mess_structs[i].inuse = 1;
			break;
		}
	}

	bcopy(message, mbox_mess_structs[i].message, length);
	mbox_mess_structs[i].msize = length;
	mbox_mess_structs[i].inuse = 1;

	if ((l = AQueueAllocLink(&mbox_mess_structs[i])) == NULL) {
		printf("ERROR: could not allocate link for message in MboxSend\n");
		exitsim();
	}

	AQueueInsertLast(&mbox_structs[handle].msg, l);

	if(LockHandleRelease(mbox_structs[handle].l) != SYNC_SUCCESS) {
		printf("Lock unable to be released in MboxSend in %d \n", GetCurrentPid());
		exitsim();
	}

	if(SemHandleSignal(mbox_structs[handle].s_full) == SYNC_FAIL) {
		printf("Bad sem signal wait in MboxSend\n");
		exitsim();
	}

	return MBOX_SUCCESS;
}

//-------------------------------------------------------
//
// int MboxRecv(mbox_t handle, int maxlength, void* message);
//
// Receive a message from the specified mailbox.  The call 
// blocks when there is no message in the buffer.  Maxlength
// should indicate the maximum number of bytes that can be
// copied from the buffer into the address of "message".  
// An error occurs if the message is larger than maxlength.
// Note that the calling process must have opened the mailbox 
// via MboxOpen.
//   
// Returns MBOX_FAIL on failure.
// Returns number of bytes written into message on success.
//
//-------------------------------------------------------
int MboxRecv(mbox_t handle, int maxlength, void* message) {
	Link *l;
	mbox_message *m;
	int cpid = GetCurrentPid();

	if (handle < 0) return MBOX_FAIL;
	if (handle > MBOX_NUM_MBOXES) return MBOX_FAIL;
	if (mbox_structs[handle].pids[cpid] == 0) {
		return MBOX_FAIL;
	}

	if(SemHandleWait(mbox_structs[handle].s_full) == SYNC_FAIL) {
		printf("Bad sem handle wait in MboxSend\n");
		exitsim();
	}

	if(LockHandleAcquire(mbox_structs[handle].l) != SYNC_SUCCESS) {
		printf("Lock unable to be acquired in MboxRecv in %d \n", GetCurrentPid());
		exitsim();
	}

	if(AQueueEmpty(&mbox_structs[handle].msg)) {
		printf("Que empty\n");
	}

	l = AQueueFirst(&mbox_structs[handle].msg);
	m = (mbox_message *) l->object;

	if(m->msize > maxlength) {
		printf("ERROR: msize (%d) > maxlength (%d)\n", m->msize, maxlength);
		return MBOX_FAIL;
	}

	bcopy(m->message, message, m->msize);

	m->inuse = 0;

	AQueueRemove(&l);

	if(LockHandleRelease(mbox_structs[handle].l) != SYNC_SUCCESS) {
		printf("Lock unable to be released in MboxSend in %d \n", GetCurrentPid());
		exitsim();
	}

	if(SemHandleSignal(mbox_structs[handle].s_empty) == SYNC_FAIL) {
		printf("Bad sem signal wait in MboxSend\n");
		exitsim();
	}

	return m->msize;

}

//--------------------------------------------------------------------------------
// 
// int MboxCloseAllByPid(int pid);
//
// Scans through all mailboxes and removes this pid from their "open procs" list.
// If this was the only open process, then it makes the mailbox available.  Call
// this function in ProcessFreeResources in process.c.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
//--------------------------------------------------------------------------------
int MboxCloseAllByPid(int pid) {
	int i;
	Link *l;
	if (pid < 0) return MBOX_FAIL;
	if (pid > MBOX_NUM_MBOXES) return MBOX_FAIL;

	for(i=0; i < MBOX_NUM_MBOXES; i++) {
		if (mbox_structs[i].pids[pid] == 1) {

			if(LockHandleAcquire(mbox_structs[i].l) != SYNC_SUCCESS) {
				printf("Lock unable to be acquired in MboxOpen in %d \n", GetCurrentPid());
				exitsim();
			}

			mbox_structs[i].pids[pid] = 0;

			if (mbox_structs[i].used == 0) {
				while(!AQueueEmpty(&mbox_structs[i].msg)) {
					l = AQueueFirst(&mbox_structs[i].msg);
					AQueueRemove(&l);
				}
				mbox_structs[i].inuse = 0;
			}

			if(LockHandleRelease(mbox_structs[i].l) != SYNC_SUCCESS) {
				printf("Lock unable to be released in MboxOpen in %d \n", GetCurrentPid());
				exitsim();
			}
		}
	}
	return MBOX_SUCCESS;
}
