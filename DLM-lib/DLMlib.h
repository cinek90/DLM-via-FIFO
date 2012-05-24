/*
 * Interfejs do DLM
 * Autorzy: Marek Jasiński, Marcin Cieślikowski, Paweł Goździkowski
 */

#ifndef DLMLIB_H_
#define DLMLIB_H_

#include <unistd.h>
#include <sys/types.h>

/* lock types */
#define CR	0
#define CW	1
#define PR	2
#define PW	3
#define EX	4

/* special timeout values */
#define NONBLOCK -1
#define TRYLOCK -2

/* responses */
#define GRANTED	0
#define TIMEDOUT	1
#define LOCKED	2
#define UNLOCKED 3
#define FREE 4
#define EOPENDLMFIFO -1
#define ECREATEFIFO -2
#define EOPENCLIENTFIFO -3
#define EWRITE -4
#define EREAD -5
#define EBADLOCKTYPE -6
#define EBADTIMEOUT -7
#define ENOTLOCKED -8
#define EAGAIN -9

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
int DLM_unlock(int resource_id);
int DLM_trylock(int resource_id, int lock_type);

#endif /* DLMLIB_H_ */
