/*
 * main.c
 */

#include <DLMlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

/*
 * Parametry:
 * 1 - numer zasobu
 * 2 - typ blokady
 * 3 - timeout
 * 4 - czas uspienia (w sekundach)
 * 5 - liczba iteracji
 */
int main(int argc, char *argv[]) {
	pid_t pid;
	int lock_response;
	int unlock_response;
	int resource_id, lock_type, sleep_time, i;
	long timeout;
	char* lock_types[] = { "CR", "CW", "PR", "PW", "EX" };
	char* responses[] =
			{ "EREAD", "EWRITE", "EOPENCLIENTFIFO", "ECREATEFIFO",
					"EOPENDLMFIFO", "GRANTED", "TIMEDOUT", "AGAIN", "LOCKED",
					"REQSENT" };

	if (argc != 6) {
		printf("Sposob uzycia:\n"
				"arg1 - numer zasobu\n"
				"arg2 - typ blokady\n"
				"arg3 - timeout\n"
				"arg4 - czas uspienia (w sekundach)\n"
				"arg5 - liczba iteracji\n");
		exit(EXIT_FAILURE);
	}

	resource_id = atoi(argv[1]);
	lock_type = atoi(argv[2]);
	timeout = atol(argv[3]);
	sleep_time = atoi(argv[4]);
	i = atoi(argv[5]);

	pid = getpid();

	while (i > 0) {
		printf(
				"%d: żądamy zasobu, resource_id: %d, lock_type: %s, timeout: %ld\n",
				pid, resource_id, lock_types[lock_type], timeout);
		// lock
		lock_response = DLM_lock(resource_id, lock_type, timeout);
		printf("%d: %s, resource_id: %d, lock_type: %s, timeout: %ld\n",
				pid, responses[lock_response + 5], resource_id,
				lock_types[lock_type], timeout);

		// sleep
		sleep(sleep_time);

		printf(
				"%d: zwalniamy zasob, resource_id: %d, lock_type: %s, timeout: %ld\n",
				pid, resource_id, lock_types[lock_type], timeout);
		// unlock
		unlock_response = DLM_unlock(resource_id);
		printf("%d: %s, resource_id: %d, lock_type: %s, timeout: %ld\n",
				pid, responses[unlock_response + 5], resource_id,
				lock_types[lock_type], timeout);
		--i;
	}

	return EXIT_SUCCESS;
}
