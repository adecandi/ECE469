#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "prod_con.h"

void main (int argc, char *argv[]) 
{
	circ_buffer *buffer1;
	uint32 h_mem;				// Handle to the shared memory page
	sem_t s_procs_completed;	// Semaphore to signal the original process that we're done
	lock_t buff_lock;			// Lock for the buffer

	int i = 0; // index for str[]
	char str[] = "Hello World";

	// Check for correct number of arguments
	if (argc != 4) { 
		Printf("Usage: "); Printf(argv[0]); Printf(" <handle_to_shared_memory_page> <handle_to_page_mapped_semaphore> <handle_to_lock>\n"); 
		Exit();
	} 

	// Convert the command-line strings into integers for use as handles
	h_mem = dstrtol(argv[1], NULL, 10); 				// The "10" means base 10
	s_procs_completed = dstrtol(argv[2], NULL, 10);		// Paged mapped semaphore
	lock_t = dstrol(argv[3], NULL, 10);					// Lock

	// Map shared memory page into this process's memory space
	if ((buffer1 = (circ_buffer *)shmat(h_mem)) == NULL) {
		Printf("Could not map the virtual address to the memory in "); Printf(argv[0]); Printf(", exiting...\n");
		Exit();
	}

	while( i < dstrlen(str)) {
		
		
		if (! (buffer1->head + 1) % BUFFER_SIZE == buffer1->tail) {
			printf("Producer X inserted: %c", str[i]);
			i++;
			buffer1->tail++;
		}
	}


	// Signal the semaphore to tell the original process that we're done
	Printf("producer: PID %d is complete.\n", getpid());
	if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
		Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
		Exit();
	}
}