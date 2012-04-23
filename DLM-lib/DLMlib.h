/*
 * DLMlib.h
 */

#ifndef DLMLIB_H_
#define DLMLIB_H_

#include <unistd.h>

#define FREERESOURCE -1

#define EOPENDLMFIFO -1
#define ECREATEFIFO -2
#define EOPENCLIENTFIFO -3
#define EWRITE -4
#define EREAD -5

#define DLM_PATH "/tmp/DLM/"
#define DLM_FIFO_PATH "/tmp/DLM/DLMfifo"

typedef struct DLMrequest {
	pid_t pid;
	int resource_id;
	int lock_type;
	long timeout;
} DLMrequest;

typedef struct DLMresponse {
	int response;
} DLMresponse;

int DLM_lock(int resource_id, int lock_type, long timeout);
int DLM_unlock( int resource_id );
int DLM_trylock( int resource_id, int lock_type );

#endif /* DLMLIB_H_ */
