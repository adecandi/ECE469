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

	char str[] = "Hello World";
	h_mem = dstrtol(argv[1], NULL, 10); // The "10" means base 10
	s_procs_completed = dstrtol(argv[2], NULL, 10);
}