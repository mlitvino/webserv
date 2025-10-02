#include "webserv.hpp"

void initServers(Data& data) {
	addrinfo hints{};
	addrinfo* srv_info = nullptr;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	for (auto& srv : data.servers) {
		try {
			srv_info = nullptr;
			srv->prepareSockFd(hints, srv_info);
			if (srv_info) {
				freeaddrinfo(srv_info);
			}
		}
		catch (const std::exception& e) {
			if (srv_info) {
				freeaddrinfo(srv_info);
			}
			throw;
		}
	}
}

void initEpoll(Data& data) {
	epoll_event& ev = data.ev;

	data.epollFd = epoll_create(DEFAULT_EPOLL_SIZE);
	if (data.epollFd == -1)
		THROW_ERRNO("epoll_create");

	ev.events = EPOLLIN;
	for (auto& srv : data.servers) {
		ev.data.ptr = static_cast<void*>(srv.get());
		int err = epoll_ctl(data.epollFd, EPOLL_CTL_ADD, srv->getSockfd(), &ev);
		if (err)
			THROW_ERRNO("epoll_ctl");
	}
}
