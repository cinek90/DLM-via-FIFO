/*
 * DLMlib.c
 */

#include "DLMlib.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/* special lock types */
#define FREERESOURCE -1

int send_request(DLMrequest* request) {
	DLMresponse response;
	int dlmfifo;
	int clientfifo;
	char path[32];

	sprintf(path, "%s%d", DLM_PATH, request->pid);
	if ((dlmfifo = open(DLM_FIFO_PATH, O_WRONLY)) < 0) {
		return EOPENDLMFIFO;
	}
	if (mkfifo(path, 00666) < 0) {
		close(dlmfifo);
		return ECREATEFIFO;
	}
	if ((clientfifo = open(path, O_RDWR)) < 0) {
		close(dlmfifo);
		unlink(path);
		return EOPENCLIENTFIFO;
	}
	if (write(dlmfifo, request, sizeof(*request)) != sizeof(*request)) {
		close(dlmfifo);
		close(clientfifo);
		unlink(path);
		return EWRITE;
	}
	if (read(clientfifo, &response, sizeof(response)) != sizeof(response)) {
		// To nie powinno sie zdarzyc
		close(dlmfifo);
		close(clientfifo);
		unlink(path);
		return EREAD;
	}
	close(dlmfifo);
	close(clientfifo);
	unlink(path);
	return response.response;
}

int DLM_lock(int resource_id, int lock_type, long timeout) {
	if (lock_type < 0 || lock_type > 4) {
		return EBADLOCKTYPE;
	}
	if (timeout < -2) {
		return EBADTIMEOUT;
	}
	DLMrequest request = { getpid(), resource_id, lock_type, timeout };
	return send_request(&request);
}

int DLM_unlock(int resource_id) {
	DLMrequest request = { getpid(), resource_id, FREERESOURCE, 0 };
	return send_request(&request);
}

int DLM_trylock(int resource_id, int lock_type) {
	if (lock_type < 0 || lock_type > 4) {
		return EBADLOCKTYPE;
	}
	DLMrequest request = { getpid(), resource_id, lock_type, TRYLOCK };
	return send_request(&request);
}
