/*
 * main.cpp
 */

#include "DLM.hpp"
#include <DLMlib.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <signal.h>

#include <iostream>
#include <cstring>
#include <map>

using namespace std;

map<int, resource_clients> resource_map;
multimap<long, client_timeout> timestamp_map;

void close_event(int);
void timeout_alarm(int);
void erase_from_timestamp_map(pid_t pid, int resource_id);
list<client>::iterator find_by_pid(list<client>& clients, pid_t pid);
int send_response(pid_t pid, int response);
void try_grant(map<int, resource_clients>::iterator iter);

void close_event(int) {
	for (int i = 0; i < NOFILE; ++i)
		close(i);
	unlink(DLM_FIFO_PATH);
	exit(EXIT_SUCCESS);
}

void timeout_alarm(int) {
	cout << "timeout alarm" << endl;
	struct timeval tv;
	struct timezone tz;
	multimap<long, client_timeout>::iterator i;
	long ts;

	for (i = timestamp_map.begin(); i != timestamp_map.end();) {
		gettimeofday(&tv, &tz); // pobranie aktualnego czasu
		ts = tv.tv_sec * 1000 + tv.tv_usec / 1000; // zamiana na ms
		if (i->first > ts)
			break;
		map<int, resource_clients>::iterator j = resource_map.find(
				i->second.resource_id);
		list<client>::iterator k = find_by_pid(j->second.waiting_clients,
				i->second.pid);
		j->second.waiting_clients.erase(k); // usuniecie klienta z kolejki oczekujacych
		cout << "usuniecie oczekujacego klienta" << endl;
		send_response(i->second.pid, TIMEDOUT);
		timestamp_map.erase(i++); // usuniecie klienta z kolejki timeout
		try_grant(j); // proba przydzielenia zasobu
	}
	if (!timestamp_map.empty()) {
		useconds_t alrm = (i->first - ts) * 1000;
		ualarm(alrm, 0);
	}
}

void erase_from_timestamp_map(pid_t pid, int resource_id) {
	for (multimap<long, client_timeout>::iterator i = timestamp_map.begin();
			i != timestamp_map.end(); ++i) {
		if (i->second.pid == pid && i->second.resource_id == resource_id) {
			timestamp_map.erase(i);
			break;
		}
	}
	// ewentualnie przestawic alarm
}

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
	if ((clientfifo = open(path, O_WRONLY)) < 0) {
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
			if (!lock_matrix[lt][i->lock_type]) // jesli blokada koliduje z zalozona blokada
				return; // to nie przydzielamy zasobu
		}
		std::cout << "przydzielamy zasob" << std::endl;
		client c = iter->second.waiting_clients.front();
		iter->second.waiting_clients.pop_front();
		erase_from_timestamp_map(c.pid, iter->first);
		iter->second.active_clients.push_back(c);
		send_response(c.pid, GRANTED);
	}
}

/*
 * Opcje programu:
 * -d - uruchamia w trybie demona
 * -l <nazwa_pliku> - logowanie do pliku
 * -h wyswietla pomoc programu
 */
int main(int argc, char* argv[]) {
	int c;
	bool daemon = false;
	bool log = false;
	char *log_filename = NULL;

	while (argc != 1 && (c = getopt(argc, argv, "hdl:")) != -1) {
		switch (c) {
		case 'd':
			daemon = true;
			break;
		case 'l':
			log = true;
			log_filename = optarg;
			break;
		default:
			cout << "Opcje programu:" << endl;
			cout << "-d - uruchamia w trybie demona" << endl;
			cout << "-l <nazwa_pliku> - logowanie do pliku" << endl;
			cout << "-h wyswietla pomoc programu" << endl;
			exit(EXIT_FAILURE);
			break;
		}
	}

	if (daemon && !log) {
		cerr << "Nie mozna uzyc DLM w trybie demona bez opcji logowania" << endl;
		exit(EXIT_FAILURE);
	}

	// logging
	int fd;
	if (log) {
		if ((fd = open(log_filename, O_WRONLY | O_CREAT)) < 0) {
			cerr << "Nie moge otworzyc pliku do logowania o nazwie: " << log_filename << endl;
			exit(EXIT_FAILURE);
		}
	}

	// demon
	if (daemon) {
		// zamkniecie deskryptorow cin cout cerr
		for (int i = 0; i < 3; ++i)
			close(i);

		// zmiana katalogu na katalog glowny
		chdir("/");

		// zerowanie maski trybu dostepu
		umask(0);

		// przejscie do pracy drugoplanowej
		if (0 != fork()) {
			exit(EXIT_SUCCESS);
		}

		// odlaczanie sie od grupy
		setpgrp();

		// ignorowanie sygnalow terminala
#ifdef SIGTTOU	/* SIGTTIN, SIDTSTP */
		signal(SIGTTOU, SIG_IGN);
#endif

		// odlaczenie sie od terminala
		int fdesc;
		if ((fdesc = open("/dev/tty", O_RDWR)) >= 0) {
			ioctl(fdesc, TIOCNOTTY, (char*) 0);
			close (fdesc);
		}
	}

	// logging
	if (log) {
		dup2(fd, 1);	// fd -> stdout
		dup2(fd, 2);	// fd -> stderr
	}

	int dlmfifo;
	if (mkdir(DLM_PATH, 00777) != 0) {
		if (errno != EEXIST) {
			cerr << "Nie moge utworzyc katalogu o nazwie: " << DLM_PATH << endl;
			return EXIT_FAILURE;
		}
	}
	if (mkfifo(DLM_FIFO_PATH, 00666) < 0) {
		if (errno != EEXIST) {
			cerr << "Nie moge utworzyc kolejki fifo o nazwie: " << DLM_FIFO_PATH << endl;
			return EXIT_FAILURE;
		}
	}
	if ((dlmfifo = open(DLM_FIFO_PATH, O_RDWR)) < 0) {
		cerr << "Nie moge otworzyc kolejki fifo o nazwie: " << DLM_FIFO_PATH << endl;
		unlink(DLM_FIFO_PATH);
		return EXIT_FAILURE;
	}

	signal(SIGALRM, timeout_alarm);
	signal(SIGTERM, close_event);
	signal(SIGINT, close_event);
	sigset_t toblock;
	sigemptyset(&toblock);
	sigaddset(&toblock, SIGALRM);

	cout << "DLM start" << endl;
	while (1) {
		DLMrequest request;
		while (read(dlmfifo, &request, sizeof(request)) != sizeof(request))
			;
		//sekcja krytyczna
		sigprocmask(SIG_BLOCK, &toblock, NULL);
		map<int, resource_clients>::iterator iter = resource_map.find(
				request.resource_id);
		if (request.lock_type == FREERESOURCE) { // zwalnianie zasobu
			cout << "żądanie zwolnienia zasobu" << endl;
			if (iter != resource_map.end()) { // znaleziono zasob
				list<client>::iterator ret = find_by_pid(
						iter->second.active_clients, request.pid);
				cout << "znaleziono zasob" << endl;
				if (ret != iter->second.active_clients.end()) { // znaleziono pid w aktywnych
					cout << "znaleziono pid w aktywnych" << endl;
					iter->second.active_clients.erase(ret); // usuniecie klienta z aktywnych (zwolnienie zasobu)
					try_grant(iter); // proba przydzielenia zasobu
				} else { // nie znaleziono pidu w aktywnych
					ret = find_by_pid(iter->second.waiting_clients,
							request.pid);
					if (ret != iter->second.waiting_clients.end()) { // znaleziono pid w oczekujacych
						cout << "znaleziono pid w oczekujacych" << endl;
						iter->second.waiting_clients.erase(ret); // usuniecie klienta z oczekujacych
						erase_from_timestamp_map(request.pid,
								request.resource_id); // usuniecie klienta z kolejki timeout
					} else { // nie znaleziono pidu w oczekujacych
						cout
								<< "nie znaleziono pidu ani w aktywnych ani w oczekujacych"
								<< endl;
						// nic do zrobienia - ktos zwalnia cos czego nie zajal
					}
				}
				if (iter->second.waiting_clients.empty()
						&& iter->second.active_clients.empty()) { // usuniecie nieuzywanego zasobu
					cout << "usuniecie nieuzywanego zasobu" << endl;
					resource_map.erase(iter);
				}
			} else { // nie znaleziono zasobu
				cout << "nie znaleziono zasobu" << endl;
				// nic do zrobienia - ktos zwalnia cos czego nie zajal i to cos nie istnieje wogole
			}
		} else { // przydzielanie zasobu
			cout << "żądanie przydzielenia zasobu" << endl;
			if (iter != resource_map.end()) { // znaleziono zasob
				cout << "zasob juz istnieje" << endl;
				// sprawdzenie czy klient nie oczekuje juz na zasob lub z niego korzysta
				list<client>::iterator ret = find_by_pid(
						iter->second.active_clients, request.pid);
				if (ret == iter->second.active_clients.end()) { // nie znaleziono pidu w aktywnych
					cout << "nie znaleziono pidu w aktywnych" << endl;
					ret = find_by_pid(iter->second.waiting_clients,
							request.pid);
					if (ret == iter->second.waiting_clients.end()) { // nie znaleziono pidu w oczekujacych
						cout << "nie znaleziono pidu w oczekujacych" << endl;
						client c(request.pid, request.lock_type);
						if (request.timeout < 0) { // ujemny timeout
							// sprawdzic, czy mozna przydzielic i od razu wyslac odpowiedz
							bool collision = false;
							for (list<client>::iterator i =
									iter->second.active_clients.begin();
									i != iter->second.active_clients.end();
									++i) {
								if (!lock_matrix[request.lock_type][i->lock_type]) { // jesli blokada koliduje z zalozona blokada
									collision = true;
									break;
								}
							}
							if (!collision) { // przydzielamy zasob
								if(request.timeout!=-2) { // funkcja trylock nie przydziela zasobu
									std::cout << "przydzielamy zasob" << std::endl;
									iter->second.active_clients.push_back(c);
								}
								send_response(c.pid, GRANTED);
							} else { // nie przydzielamy zasobu
								send_response(c.pid, LOCKED);
							}
						} else { // nieujemny timeout
							iter->second.waiting_clients.push_back(c); // dodanie klienta na koniec kolejki oczekujacych
							if (request.timeout > 0) { // dodanie do kolejki timeout o ile timeout > 0
								client_timeout ct(request.pid,
										request.resource_id);
								struct timeval tv;
								struct timezone tz;
								gettimeofday(&tv, &tz); // pobranie aktualnego czasu
								long ts = tv.tv_sec * 1000 + tv.tv_usec / 1000; // zamiana na ms
								multimap<long, client_timeout>::iterator current =
										timestamp_map.insert(
												make_pair(ts + request.timeout,
														ct));
								if (current == timestamp_map.begin())
									ualarm(
											(timestamp_map.begin()->first - ts)
													* 1000, 0); // przestawianie alarmu
							}
							try_grant(iter); // proba przydzielenia zasobu
						}
					} else { // znaleziono pid w oczekujacych
						cout << "znaleziono pid w oczekujacych" << endl;
						send_response(request.pid, AGAIN);
					}
				} else { // znaleziono pid w aktywnych
					cout << "znaleziono pid w aktywnych" << endl;
					send_response(request.pid, AGAIN);
				}
			} else { // nie znaleziono zasobu
				cout << "zasob jeszcze nie istnieje" << endl;
				if(request.timeout!=-2) { //funkcja try_lock nie przydziela zasobu
					// stworz id zasobu i przydziel zasob
					client c(request.pid, request.lock_type);
					resource_clients rc;
					rc.active_clients.push_back(c);
					resource_map[request.resource_id] = rc;
				}
				// wyslanie odpowiedzi
				send_response(request.pid, GRANTED);
			}
		}
		// koniec sekcji krytycznej
		sigprocmask(SIG_UNBLOCK, &toblock, NULL);
	}

	return EXIT_SUCCESS;
}
