/*
 * main.cpp
 */

#include "DLM.hpp"
#include "DLMlib.h"
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <map>

int end = 0;

using namespace std;

list<client>::iterator find_by_pid(list<client>& clients, pid_t pid) {
	list<client>::iterator ret = clients.begin();
	list<client>::iterator end = clients.end();
	for (; ret != end; ++ret) {
		if (ret->pid == pid)
			return ret;
	}
	return ret;
}

int send_response(pid_t pid, int response) {
	DLMresponse resp;
	resp.response = response;
	int clientfifo;
	char path[32];

	sprintf(path, "%s%d", DLM_PATH, pid);
	if ((clientfifo = open(path, O_WRONLY | O_NDELAY))) {
		return EOPENCLIENTFIFO;
	}
	if (write(clientfifo, &resp, sizeof(resp)) != sizeof(resp)) {
		// To nie powinno sie zdarzyc
		close(clientfifo);
		return EWRITE;
	}
	close(clientfifo);
	return 0;
}

void try_grant(map<int, resource_clients>::iterator iter) {
	while (!iter->second.waiting_clients.empty()) {
		int lt = iter->second.waiting_clients.front().lock_type;
		for (list<client>::iterator i = iter->second.active_clients.begin();
				i != iter->second.active_clients.end(); ++i) {
			if (!lock_matrix[lt][i->lock_type])
				return;
		}
		client c = iter->second.waiting_clients.front();
		iter->second.waiting_clients.pop_front();
		iter->second.active_clients.push_back(c);
		send_response(c.pid, GRANTED);
	}
}

int main() {
	int dlmfifo;
	if (mkdir(DLM_PATH, 00777) != 0) {
		if (errno != EEXIST) {
			fprintf(stderr, "Nie moge utworzyc katalogu %s\n", DLM_PATH);
			return EXIT_FAILURE;
		}
	}
	if (mkfifo(DLM_FIFO_PATH, 00666) < 0) {
		if (errno != EEXIST) {
			fprintf(stderr, "Nie moge utworzyc kolejki fifo %s\n",
					DLM_FIFO_PATH);
			return EXIT_FAILURE;
		}
	}
	if ((dlmfifo = open(DLM_FIFO_PATH, O_RDONLY | O_NDELAY) < 0)) {
		unlink(DLM_FIFO_PATH);
		fprintf(stderr, "Nie moge otworzyc kolejki FIFO %s\n", DLM_FIFO_PATH);
		return EXIT_FAILURE;
	}

	map<int, resource_clients> resource_map;
	multimap<long, client_timeout> timestamp_map;
	DLMrequest request;

	while (end == 0) {
		cout << "start" << endl;
		read(dlmfifo, &request, sizeof(request));
		// TODO sekcja krytyczna
		map<int, resource_clients>::iterator iter = resource_map.find(
				request.resource_id);
		if (request.lock_type == FREERESOURCE) { // zwalnianie zasobu
			if (iter != resource_map.end()) { // znaleziono zasob
				list<client>::iterator ret = find_by_pid(
						iter->second.active_clients, request.pid);
				if (ret != iter->second.active_clients.end()) { // znaleziono w aktywnych
					iter->second.active_clients.erase(ret);
					try_grant(iter);
				} else {	// nie znaleziono w aktywnych
					ret = find_by_pid(iter->second.waiting_clients,
							request.pid);
					if (ret != iter->second.waiting_clients.end()) { // znaleziono w oczekujacych
						iter->second.waiting_clients.erase(ret);
						try_grant(iter);
					} else {	// nie znaleziono w oczekujacych
						// nic do zrobienia - ktos zwalnia cos czego nie zajal
					}
				}
			} else {	// nie znaleziono zasobu
				// nic do zrobienia - ktos zwalnia cos czego nie zajal i to cos nie istnieje wogole
			}
		} else { // przydzielanie zasobu
			if (iter != resource_map.end()) { // znaleziono zasob
					// sprawdzenie czy klient nie oczekuje juz na zasob lub z niego korzysta
				list<client>::iterator ret = find_by_pid(
						iter->second.active_clients, request.pid);
				if (ret == iter->second.active_clients.end()) {	// nie znaleziono go w aktywnych
					ret = find_by_pid(iter->second.waiting_clients,
							request.pid);
					if (ret == iter->second.waiting_clients.end()) {	// nie znaleziono go w oczekujacych
						client c(request.pid, request.lock_type);
						iter->second.waiting_clients.push_back(c);
						try_grant(iter);
					} else {	// znaleziono go w oczekujacych
						send_response(request.pid, AGAIN);
					}
				} else {	// znaleziono go w aktywnych
					send_response(request.pid, AGAIN);
				}
			} else { // nie znaleziono zasobu
				// stworz id zasobu i przydziel zasob
				client c(request.pid, request.lock_type);
				resource_clients rc;
				rc.active_clients.push_back(c);
				resource_map[request.resource_id] = rc;
				// wyslanie odpowiedzi
				send_response(request.pid, GRANTED);
			}
		}
		// TODO koniec sekcji krytycznej
	}

	close(dlmfifo);
	unlink(DLM_FIFO_PATH);
	return EXIT_SUCCESS;
}

