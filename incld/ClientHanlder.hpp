#pragma once

#include <cstring>
#include <memory>

#include "webserv.hpp"
#include "IEpollFdOwner.hpp"

enum class ClientState {
	READING_REQUEST,
	WRITING_RESPONSE,
	READING_FILE,
	WRITING_FILE,

	READING_CLIENT_HEADER,
	READING_CLIENT_BODY,
	SENDING_RESPONSE,
	SENDING_FILE,
	GETTING_FILE,
};

class Server;

class ClientHandler : public IEpollFdOwner {
private:
	sockaddr_storage _clientAddr{};
	socklen_t _clientAddrLen = 0;
	Server& _owner;
	size_t _index = 0;

	int _sockFd = -1;
	int _fileFd = -1;

	ClientState _state = ClientState::READING_REQUEST;

	std::string _buffer;
	std::string _headRequest;
	std::string _body;

	// HTTP request data
	std::string _httpMethod;
	std::string _httpPath;
	std::string _httpVersion;
	std::string _responseBuffer;

public:
	~ClientHandler();
	explicit ClientHandler(Server& owner);

	// Delete copy constructor and assignment
	ClientHandler(const ClientHandler&) = delete;
	ClientHandler& operator=(const ClientHandler&) = delete;

	// Move constructor (move assignment disabled due to reference member)
	// ClientHandler(ClientHandler&& other) noexcept;

	void setIndex(size_t index);
	void CloseConnection(int epoll_fd);

	void acceptConnect(int srvSockFd, int epoll_fd);
	void readAll();

	// HTTP handling methods
	bool readHttpRequest();  // Returns false if client disconnected
	bool isCompleteRequest();
	void processHttpRequest();
	void sendHttpResponse(int epoll_fd);
	std::string parseRequestLine(const std::string& line);
	std::string generateHttpResponse(const std::string& filePath, int statusCode);
	std::string readFile(const std::string& filePath);
	std::string getMimeType(const std::string& filePath);

	// HTTP method handlers
	void handleGetRequest(const std::string& path);
	void handlePostRequest(const std::string& path);
	void handleDeleteRequest(const std::string& path);

	// Configuration validation methods
	bool isMethodAllowed(const std::string& method, const std::string& path);
	bool isBodySizeValid();
	std::string getCustomErrorPage(int statusCode);
	std::string findIndexFile(const std::string& path);

	// Utility functions
	std::string intToString(int value);

	void handleEpollEvent(epoll_event& ev, int epoll_fd, int eventFd);

private:
	ClientHandler() = delete; // Prevent default construction
};


