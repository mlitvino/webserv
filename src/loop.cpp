#include "webserv.hpp"

int parseRequest(HttpRequest& req, std::string& use_buf) {
	int leftBytes = 0;

	// Extract the method string first
	std::string methodStr = use_buf.substr(0, 3);
	if (methodStr == "GET") {
		req.method = HttpMethod::GET;
	} else if (methodStr == "POS") { // "POST" but we only got 3 chars
		req.method = HttpMethod::POST;
	} else if (methodStr == "DEL") { // "DELETE" but we only got 3 chars  
		req.method = HttpMethod::DELETE;
	} else {
		THROW("unknown method in request");
	}
	use_buf.erase(0, 3);

	return leftBytes;
}

void acceptingLoop(Data& data) {
	int nbr_events;

	initEpoll(data);
	std::cout << "Epoll fd: " << data.epollFd << std::endl;
	
	while (true) {
		nbr_events = epoll_wait(data.epollFd, data.events, MAX_EVENTS, -1);
		if (nbr_events == -1)
			THROW_ERRNO("epoll_wait");

		for (int i = 0; i < nbr_events; ++i) {
			if (data.events[i].events == EPOLLIN)
				std::cout << "READING EPOLL EVENT" << std::endl;
			else if (data.events[i].events == EPOLLOUT)
				std::cout << "WRITING EPOLL EVENT" << std::endl;

			auto* owner = static_cast<IEpollFdOwner*>(data.events[i].data.ptr);
			owner->handleEpollEvent(data.ev, data.epollFd);
		}
	}
	
	close(data.epollFd);
}
