#ifndef __USERPROG__
#define __USERPROG__

typedef struct buffer {
	char buffer[BUFFERSIZE];	// Buffer
	int head;					// head pointer
	int tail;					// tail pointer

} circ_buffer;

#define FILE_PROD "producer.dlx.obj"
#define FILE_CONS "consumer.dlx.obj"


#endif
