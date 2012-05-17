/*
 * DLM.hpp
 */

#ifndef DLM_HPP_
#define DLM_HPP_

#include <unistd.h>
#include <sys/types.h>

#include <list>

const bool lock_matrix[][5] = { { 1, 1, 1, 1, 0 },
								{ 1, 1, 0, 0, 0 },
								{ 1, 0, 1, 0, 0 },
								{ 1, 0, 0, 0, 0 },
								{ 0, 0, 0, 0, 0 } };

struct client {
	client(pid_t p = 0, int lt = 0) : pid(p), lock_type(lt) { }
	pid_t pid;
	int lock_type;
};

struct client_timeout {
	client_timeout(pid_t p, int rid) : pid(p), resource_id(rid) { }
	pid_t pid;
	int resource_id;
};

struct resource_clients {
	std::list<client> active_clients;
	std::list<client> waiting_clients;
};

#endif /* DLM_HPP_ */
