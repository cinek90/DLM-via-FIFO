/*
 * DLMlib.c
 */

#include "DLMlib.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int DLM_lock(int resource_id, int lock_type, long timeout) {
	DLMrequest request = { getpid(), resource_id, lock_type, timeout };
	DLMresponse response;
	int dlmfifo;
	int clientfifo;
	char path[32];

	sprintf(path, "%s%d", DLM_PATH, request.pid);
	if ((dlmfifo = open(DLM_FIFO_PATH, O_WRONLY)) < 0) {
		return EOPENDLMFIFO;
	}
	if (mkfifo(path, S_IRUSR | S_IWUSR) < 0) {
		close(dlmfifo);
		return ECREATEFIFO;
	}
	if ((clientfifo = open(path, O_RDONLY))) {
		close(dlmfifo);
		unlink(path);
		return EOPENCLIENTFIFO;
	}
	if (write(dlmfifo, &request, sizeof(request)) != sizeof(request) ) {
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

int DLM_unlock( int resource_id ) {
	DLMrequest request = { getpid(), resource_id, FREERESOURCE, 0 };
	int dlmfifo;

	if ((dlmfifo = open(DLM_FIFO_PATH, O_WRONLY)) < 0) {
		return EOPENDLMFIFO;
	}
	if (write(dlmfifo, &request, sizeof(request)) != sizeof(request) ) {
		close(dlmfifo);
		return EWRITE;
	}
	close(dlmfifo);
	return 0;
}

int DLM_trylock( int resource_id, int lock_type ) {
	return DLM_lock(resource_id, lock_type, -1);
}
