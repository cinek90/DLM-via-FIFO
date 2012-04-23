/*
 * dlm.c
 */
#include "DLMlib.h"
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
int exit=0;
int main() {
	int dlmfifo;
	if (mkdir(DLM_PATH, 00777) != 0) {
		if (errno != EEXIST) {
			fprinf(stderr, "Nie moge utworzyc katalogu %s\n", DLM_PATH);
			return EXIT_FAILURE;
		}
	}
	if (mkfifo(DLM_FIFO_PATH, 00622) < 0) {
		fprinf(stderr, "Nie moge utworzyc kolejki FIFO %s\n", DLM_FIFO_PATH);
		return EXIT_FAILURE;
	}
	if ((dlmfifo = open(DLM_FIFO_PATH, O_RDONLY))) {
		unlink(DLM_FIFO_PATH);
		fprinf(stderr, "Nie moge otworzyc kolejki FIFO %s\n", DLM_FIFO_PATH);
		return EXIT_FAILURE;
	}
	while(exit==0) {

	}
	return EXIT_SUCCESS;
}
