#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "prod_cons.h"

void main (int argc, char *argv[]) 
{
	circ_buffer *buffer1;
	uint32 h_mem;				// Handle to the shared memory page
	sem_t s_procs_completed;	// Semaphore to signal the original process that we're done
	lock_t buff_lock;			// Lock for the buffer

	cond_t c_full;				// Full slots
	cond_t c_empty;				// Empty slots

	int i = 0; // index for str[]
	char str[] = "Hello World";
	int length = dstrlen(str);

	// Check for correct number of arguments
	if (argc != 6) { 
		Printf("Usage: "); Printf(argv[0]); Printf(" <handle_to_shared_memory_page> <handle_to_page_mapped_semaphore> <handle_to_lock> <handle to c_full> <handle to c_empty>\n"); 
		Exit();
	} 

	// Convert the command-line strings into integers for use as handles
	h_mem = dstrtol(argv[1], NULL, 10); 				// The "10" means base 10
	s_procs_completed = dstrtol(argv[2], NULL, 10);		// Paged mapped semaphore
	buff_lock = dstrtol(argv[3], NULL, 10);				// Lock
	c_full = dstrtol(argv[4], NULL, 10);				// full slots
	c_empty = dstrtol(argv[5], NULL, 10);				// empty slots

	// Map shared memory page into this process's memory space
	if ((buffer1 = (circ_buffer *)shmat(h_mem)) == NULL) {
		Printf("Could not map the virtual address to the memory in "); Printf(argv[0]); Printf(", exiting...\n");
		Exit();
	}

	for(i = 0; i < length; i++) {
		// Acquire the lock for the consumer
		while(lock_acquire(buff_lock) != SYNC_SUCCESS);
		// Check if empty or not
		while(buffer1->head == buffer1->tail) {
			cond_wait(c_empty);
		}
		Printf("Consumer %d removed: %c\n", getpid(), buffer1->buffer[buffer1->tail]);
		buffer1->tail = (buffer1->tail + 1) % BUFFERSIZE;
		cond_signal(c_full);
		while(lock_release(buff_lock) != SYNC_SUCCESS);
	}

	// Signal the semaphore to tell the original process that we're done
	Printf("Consumer: PID %d is complete.\n", getpid());
	if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
		Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
		Exit();
	}
}
