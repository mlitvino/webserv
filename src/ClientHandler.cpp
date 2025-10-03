#include "ClientHanlder.hpp"
#include "Server.hpp"
#include <sstream>
#include <fstream>
#include <algorithm>

void ClientHandler::acceptConnect(int srvSockFd, int epoll_fd) {
	epoll_event ev;

	_sockFd = accept(srvSockFd, reinterpret_cast<sockaddr*>(&_clientAddr), &_clientAddrLen);
	if (_sockFd == -1)
		THROW_ERRNO("accept");

	int flags = fcntl(_sockFd, F_GETFL, 0);
	fcntl(_sockFd, F_SETFL, flags | O_NONBLOCK);

	// Initially only register for EPOLLIN (reading) events
	ev.events = EPOLLIN;
	ev.data.ptr = static_cast<void*>(this);
	int err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, _sockFd, &ev);
	if (err)
		THROW_ERRNO("epoll_ctl");
	_state = ClientState::READING_REQUEST;
}

void ClientHandler::readAll() {
	// Legacy method - keeping for compatibility
}

bool ClientHandler::readHttpRequest() {
	char buffer[IO_BUFFER_SIZE];
	int bytesRead = read(_sockFd, buffer, sizeof(buffer) - 1);

	if (bytesRead > 0) {
		buffer[bytesRead] = '\0';
		_buffer.append(buffer, bytesRead);
		std::cout << "DEBUG: Received " << bytesRead << " bytes" << std::endl;
		std::cout << "DEBUG: Total buffer size now: " << _buffer.size() << std::endl;
		std::cout << "DEBUG: New data: [" << std::string(buffer, bytesRead) << "]" << std::endl;
		return true;  // Successfully read data
	}
	else if (bytesRead == 0) {
		std::cout << "DEBUG: Client disconnected" << std::endl;
		return false;  // Client disconnected
	}
	else if (bytesRead == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			std::cout << "DEBUG: Read error: " << strerror(errno) << std::endl;
			THROW_ERRNO("read");
		} else {
			std::cout << "DEBUG: Read would block (EAGAIN/EWOULDBLOCK)" << std::endl;
		}
		return true;  // Not an error, just would block
	}
	return true;
}

bool ClientHandler::isCompleteRequest() {
	// HTTP request is complete when we find double CRLF
	bool isComplete = _buffer.find("\r\n\r\n") != std::string::npos;
	std::cout << "DEBUG: isCompleteRequest() = " << (isComplete ? "true" : "false") << std::endl;
	std::cout << "DEBUG: Buffer size = " << _buffer.size() << std::endl;
	std::cout << "DEBUG: Buffer content (first 200 chars): " << _buffer.substr(0, 200) << std::endl;

	// Also check for just \n\n in case client doesn't send \r\n
	if (!isComplete) {
		isComplete = _buffer.find("\n\n") != std::string::npos;
		std::cout << "DEBUG: Checking for \\n\\n: " << (isComplete ? "true" : "false") << std::endl;
	}

	return isComplete;
}

std::string ClientHandler::parseRequestLine(const std::string& line) {
	size_t firstSpace = line.find(' ');
	size_t secondSpace = line.find(' ', firstSpace + 1);

	if (firstSpace == std::string::npos || secondSpace == std::string::npos)
		return "";

	_httpMethod = line.substr(0, firstSpace);
	_httpPath = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
	_httpVersion = line.substr(secondSpace + 1);

	// Remove \r\n from version
	size_t crPos = _httpVersion.find('\r');
	if (crPos != std::string::npos)
		_httpVersion = _httpVersion.substr(0, crPos);

	return _httpPath;
}

void ClientHandler::processHttpRequest() {
	std::cout << "DEBUG: processHttpRequest() called" << std::endl;

	if (_buffer.empty()) {
		std::cout << "DEBUG: Buffer is empty!" << std::endl;
		return;
	}

	std::cout << "DEBUG: Buffer content: " << _buffer << std::endl;

	// Parse the first line (request line)
	size_t firstLine = _buffer.find("\r\n");
	if (firstLine == std::string::npos) {
		std::cout << "DEBUG: No CRLF found in buffer!" << std::endl;
		return;
	}

	std::string requestLine = _buffer.substr(0, firstLine);
	std::cout << "DEBUG: Request line: " << requestLine << std::endl;

	std::string requestedPath = parseRequestLine(requestLine);

	std::cout << "Method: " << _httpMethod << ", Path: " << _httpPath << ", Version: " << _httpVersion << std::endl;

	// Check body size for POST requests
	if (_httpMethod == "POST") {
		if (!isBodySizeValid()) {
			_responseBuffer = generateHttpResponse("", 413);
			return;
		}
	}

	// Check if method is allowed for this path
	if (!isMethodAllowed(_httpMethod, requestedPath)) {
		_responseBuffer = generateHttpResponse("", 405);
		return;
	}

	// Handle different HTTP methods
	if (_httpMethod == "GET") {
		handleGetRequest(requestedPath);
	}
	else if (_httpMethod == "POST") {
		handlePostRequest(requestedPath);
	}
	else if (_httpMethod == "DELETE") {
		handleDeleteRequest(requestedPath);
	}
	else {
		// Method not allowed
		_responseBuffer = generateHttpResponse("", 405);
	}
}

void ClientHandler::sendHttpResponse(int epoll_fd) {
	std::cout << "DEBUG: sendHttpResponse() called" << std::endl;
	std::cout << "DEBUG: Response buffer size: " << _responseBuffer.size() << std::endl;

	if (_responseBuffer.empty()) {
		std::cout << "DEBUG: Response buffer is empty!" << std::endl;
		// Create a simple default response for testing
		_responseBuffer = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 25\r\n\r\n<h1>Hello from Webserv!</h1>";
		std::cout << "DEBUG: Created default response" << std::endl;
	}

	std::cout << "DEBUG: Response content (first 200 chars): " << _responseBuffer.substr(0, 200) << std::endl;

	int bytesSent = send(_sockFd, _responseBuffer.c_str(), _responseBuffer.length(), 0);

	std::cout << "DEBUG: send() returned: " << bytesSent << std::endl;

	if (bytesSent > 0) {
		std::cout << "Sent " << bytesSent << " bytes successfully" << std::endl;

		// Check if we sent a "Connection: close" response
		bool shouldCloseConnection = _responseBuffer.find("Connection: close") != std::string::npos;

		_buffer.clear();
		_responseBuffer.clear();

		if (shouldCloseConnection) {
			std::cout << "DEBUG: Connection: close header detected, closing connection" << std::endl;
			CloseConnection(epoll_fd);
			return;
		}

		// Otherwise, keep connection alive for next request
		_state = ClientState::READING_REQUEST;

		// Switch back to EPOLLIN for reading next request
		epoll_event new_ev;
		new_ev.events = EPOLLIN;
		new_ev.data.ptr = static_cast<void*>(this);
		int err = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, _sockFd, &new_ev);
		if (err) {
			std::cout << "DEBUG: epoll_ctl MOD back to EPOLLIN failed: " << strerror(errno) << std::endl;
			THROW_ERRNO("epoll_ctl MOD back to EPOLLIN");
		}

		std::cout << "DEBUG: Response sent, switched back to EPOLLIN for next request" << std::endl;
	}
	else if (bytesSent == -1) {
		std::cout << "DEBUG: send() failed, errno: " << errno << std::endl;
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			THROW_ERRNO("send");
	}
	else if (bytesSent == 0) {
		std::cout << "DEBUG: send() returned 0 - connection closed" << std::endl;
	}
}

void ClientHandler::handleEpollEvent(epoll_event& ev, int epoll_fd, int eventFd) {
	std::cout << "DEBUG: handleEpollEvent called with events: " << ev.events << std::endl;
	std::cout << "DEBUG: Current state: " << static_cast<int>(_state) << std::endl;

	// Check for error conditions first
	if (ev.events & (EPOLLHUP | EPOLLERR)) {
		std::cout << "DEBUG: Connection error or hangup detected, closing connection" << std::endl;
		CloseConnection(epoll_fd);
		return;
	}

	switch (_state) {
		case ClientState::READING_REQUEST: {
			if (ev.events & EPOLLIN) {
				std::cout << "DEBUG: EPOLLIN event - reading request" << std::endl;
				try {
					if (!readHttpRequest()) {
						// Client disconnected
						std::cout << "DEBUG: Client disconnected, closing connection" << std::endl;
						CloseConnection(epoll_fd);
						return;  // Don't process further
					}
				} catch (const std::exception& e) {
					std::cout << "DEBUG: Exception in readHttpRequest: " << e.what() << std::endl;
					CloseConnection(epoll_fd);
					return;
				}

				if (isCompleteRequest()) {
					processHttpRequest();
					_state = ClientState::WRITING_RESPONSE;

					// Change epoll registration to EPOLLOUT for writing
					epoll_event new_ev;
					new_ev.events = EPOLLOUT;
					new_ev.data.ptr = static_cast<void*>(this);
					int err = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, _sockFd, &new_ev);
					if (err) {
						std::cout << "DEBUG: epoll_ctl MOD failed: " << strerror(errno) << std::endl;
						THROW_ERRNO("epoll_ctl MOD");
					}
					std::cout << "DEBUG: Successfully switched to EPOLLOUT for writing response" << std::endl;
				}
			}
			if (ev.events & EPOLLOUT) {
				std::cout << "DEBUG: EPOLLOUT event while in READING_REQUEST state (ignoring)" << std::endl;
			}
			break;
		}
		case ClientState::WRITING_RESPONSE: {
			if (ev.events & EPOLLOUT) {
				std::cout << "DEBUG: EPOLLOUT event - sending response" << std::endl;
				sendHttpResponse(epoll_fd);
			}
			if (ev.events & EPOLLIN) {
				std::cout << "DEBUG: EPOLLIN event while in WRITING_RESPONSE state" << std::endl;
				std::cout << "WRITING EPOLL EVENT" << std::endl;
				// Try to send the response anyway - the socket might be ready for writing
				std::cout << "DEBUG: Attempting to send response despite EPOLLIN event" << std::endl;
				sendHttpResponse(epoll_fd);
			}
			break;
		}
		case ClientState::READING_FILE: {
			if (ev.events & EPOLLIN) {
				// File reading logic would go here
			}
			break;
		}
		case ClientState::WRITING_FILE: {
			if (ev.events & EPOLLOUT) {
				// File writing logic would go here
			}
			break;
		}
		default: {
			THROW("unknown ClientHandler state");
			break;
		}
	}
}

void ClientHandler::CloseConnection(int epoll_fd) {
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, _sockFd, nullptr))
		THROW("epoll_ctl(DEL)");
	close(_sockFd);

	_owner.RemoveClientHandler(_index);

	std::cout << "CLOSED" << std::endl;
}

void ClientHandler::setIndex(size_t index) {
	_index = index;
}

ClientHandler::ClientHandler(Server& owner)
	: _clientAddrLen(sizeof(_clientAddr))
	, _owner(owner)
	, _index(owner.getSizeClients()) {
	_buffer.reserve(IO_BUFFER_SIZE);
}

// Move constructor
ClientHandler::ClientHandler(ClientHandler&& other) noexcept
	: _clientAddr(other._clientAddr)
	, _clientAddrLen(other._clientAddrLen)
	, _owner(other._owner)
	, _index(other._index)
	, _sockFd(other._sockFd)
	, _fileFd(other._fileFd)
	, _state(other._state)
	, _buffer(std::move(other._buffer))
	, _headRequest(std::move(other._headRequest))
	, _body(std::move(other._body))
	, _httpMethod(std::move(other._httpMethod))
	, _httpPath(std::move(other._httpPath))
	, _httpVersion(std::move(other._httpVersion))
	, _responseBuffer(std::move(other._responseBuffer)) {
	other._sockFd = -1;
	other._fileFd = -1;
}

ClientHandler::~ClientHandler() {
	if (_sockFd != -1)
		close(_sockFd);
	if (_fileFd != -1)
		close(_fileFd);
}

void ClientHandler::handleGetRequest(const std::string& path) {
	std::cout << "DEBUG: handleGetRequest() called with path: " << path << std::endl;

	std::string filePath = path;

	// If path is "/" or ends with "/", try to serve index file
	if (filePath == "/" || (!filePath.empty() && filePath.back() == '/')) {
		std::string indexFile = findIndexFile(path);
		filePath += indexFile;
		std::cout << "DEBUG: Added " << indexFile << ", filePath now: " << filePath << std::endl;
	}

	// Remove leading slash and prepend document root
	if (!filePath.empty() && filePath[0] == '/')
		filePath = filePath.substr(1);

	std::string fullPath = "web/www/" + filePath;
	std::cout << "DEBUG: Full file path: " << fullPath << std::endl;

	// Check if file exists and generate response
	std::ifstream file(fullPath);
	if (file.good()) {
		std::cout << "DEBUG: File exists, generating 200 response" << std::endl;
		file.close();
		_responseBuffer = generateHttpResponse(fullPath, 200);
	}
	else {
		std::cout << "DEBUG: File not found, generating 404 response" << std::endl;
		_responseBuffer = generateHttpResponse("web/www/errors/404.html", 404);
	}

	std::cout << "DEBUG: Response buffer size after generation: " << _responseBuffer.size() << std::endl;
}

void ClientHandler::handlePostRequest(const std::string& path) {
	std::cout << "DEBUG: handlePostRequest() called with path: " << path << std::endl;

	// Extract POST body data if present
	std::string bodyData;
	size_t bodyStart = _buffer.find("\r\n\r\n");
	if (bodyStart != std::string::npos) {
		bodyData = _buffer.substr(bodyStart + 4);
		std::cout << "DEBUG: POST body data: " << bodyData << std::endl;
	}

	// Generate response content
	std::string content = "<html><body>";
	content += "<h1>POST Method Received</h1>";
	content += "<p><strong>Path:</strong> " + path + "</p>";

	if (!bodyData.empty()) {
		content += "<p><strong>Body Data:</strong></p>";
		content += "<pre>" + bodyData + "</pre>";
	} else {
		content += "<p><em>No body data received</em></p>";
	}

	content += "<p><a href=\"/\">← Back to Home</a></p>";
	content += "</body></html>";

	// Build complete HTTP response with proper headers
	_responseBuffer = "HTTP/1.1 200 OK\r\n";
	_responseBuffer += "Content-Type: text/html; charset=UTF-8\r\n";
	_responseBuffer += "Content-Length: " + intToString(content.length()) + "\r\n";
	_responseBuffer += "Server: Webserv/1.0\r\n";
	_responseBuffer += "Connection: close\r\n";
	_responseBuffer += "Cache-Control: no-cache\r\n";
	_responseBuffer += "\r\n";
	_responseBuffer += content;

	std::cout << "DEBUG: POST response generated, size: " << _responseBuffer.size() << std::endl;
}

void ClientHandler::handleDeleteRequest(const std::string& path) {
	std::cout << "DEBUG: handleDeleteRequest() called with path: " << path << std::endl;

	// Generate response content
	std::string content = "<html><body>";
	content += "<h1>DELETE Method Received</h1>";
	content += "<p><strong>Path:</strong> " + path + "</p>";
	content += "<p><em>Note: This is a placeholder response. Actual file deletion would be implemented here.</em></p>";
	content += "<p><a href=\"/\">← Back to Home</a></p>";
	content += "</body></html>";

	// Build complete HTTP response with proper headers
	_responseBuffer = "HTTP/1.1 200 OK\r\n";
	_responseBuffer += "Content-Type: text/html; charset=UTF-8\r\n";
	_responseBuffer += "Content-Length: " + intToString(content.length()) + "\r\n";
	_responseBuffer += "Server: Webserv/1.0\r\n";
	_responseBuffer += "Connection: close\r\n";
	_responseBuffer += "Cache-Control: no-cache\r\n";
	_responseBuffer += "\r\n";
	_responseBuffer += content;

	std::cout << "DEBUG: DELETE response generated, size: " << _responseBuffer.size() << std::endl;
}

std::string ClientHandler::generateHttpResponse(const std::string& filePath, int statusCode) {
	std::string response;
	std::string statusText;
	std::string content;

	// Set status text based on status code
	switch (statusCode) {
		case 200: statusText = "OK"; break;
		case 404: statusText = "Not Found"; break;
		case 405: statusText = "Method Not Allowed"; break;
		case 413: statusText = "Payload Too Large"; break;
		case 500: statusText = "Internal Server Error"; break;
		default: statusText = "Unknown"; break;
	}

	// Read file content if file path is provided
	if (!filePath.empty()) {
		content = readFile(filePath);
		if (content.empty() && statusCode == 200) {
			// File couldn't be read, return 500 error
			statusCode = 500;
			statusText = "Internal Server Error";
			content = "<html><body><h1>500 Internal Server Error</h1></body></html>";
		}
		else if (content.empty() && statusCode == 404) {
			// Try to get custom 404 error page
			std::string customErrorPage = getCustomErrorPage(404);
			if (!customErrorPage.empty()) {
				content = readFile(customErrorPage);
			}
			if (content.empty()) {
				// Use default 404 page
				content = "<html><body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body></html>";
			}
		}
	}
	else {
		// No file path, generate default content based on status
		switch (statusCode) {
			case 404: {
				// Try to get custom 404 error page
				std::string customErrorPage = getCustomErrorPage(404);
				if (!customErrorPage.empty()) {
					content = readFile(customErrorPage);
				}
				if (content.empty()) {
					content = "<html><body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body></html>";
				}
				break;
			}
			case 405:
				content = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
				break;
			case 413:
				content = "<html><body><h1>413 Payload Too Large</h1><p>The request entity is larger than limits defined by server.</p></body></html>";
				break;
			case 500: {
				// Try to get custom 500 error page
				std::string customErrorPage = getCustomErrorPage(500);
				if (!customErrorPage.empty()) {
					content = readFile(customErrorPage);
				}
				if (content.empty()) {
					content = "<html><body><h1>500 Internal Server Error</h1></body></html>";
				}
				break;
			}
			default:
				content = "<html><body><h1>Error</h1></body></html>";
				break;
		}
	}

	// Build HTTP response
	response = "HTTP/1.1 " + intToString(statusCode) + " " + statusText + "\r\n";
	response += "Content-Type: " + getMimeType(filePath) + "\r\n";
	response += "Content-Length: " + intToString(content.length()) + "\r\n";
	response += "Server: Webserv/1.0\r\n";
	response += "Connection: close\r\n";
	response += "\r\n";
	response += content;

	return response;
}

std::string ClientHandler::readFile(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open())
		return "";

	std::string content;
	std::string line;

	while (std::getline(file, line)) {
		content += line + "\n";
	}

	file.close();
	return content;
}

std::string ClientHandler::getMimeType(const std::string& filePath) {
	if (filePath.empty())
		return "text/html";

	size_t dotPos = filePath.find_last_of('.');
	if (dotPos == std::string::npos)
		return "text/plain";

	std::string extension = filePath.substr(dotPos + 1);

	// Convert to lowercase
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	if (extension == "html" || extension == "htm")
		return "text/html";
	else if (extension == "css")
		return "text/css";
	else if (extension == "js")
		return "application/javascript";
	else if (extension == "json")
		return "application/json";
	else if (extension == "txt")
		return "text/plain";
	else if (extension == "png")
		return "image/png";
	else if (extension == "jpg" || extension == "jpeg")
		return "image/jpeg";
	else if (extension == "gif")
		return "image/gif";
	else if (extension == "ico")
		return "image/x-icon";
	else
		return "application/octet-stream";
}

std::string ClientHandler::intToString(int value) {
        return std::to_string(value);
}

// Check if the HTTP method is allowed for the given path
bool ClientHandler::isMethodAllowed(const std::string& method, const std::string& path) {
	std::cout << "DEBUG: isMethodAllowed() called with method: " << method << ", path: " << path << std::endl;

	// Get server configuration
	const std::vector<Location>& locations = _owner.getLocations();
	std::cout << "DEBUG: Found " << locations.size() << " locations" << std::endl;

	// Find matching location
	const Location* matchedLocation = nullptr;
	size_t longestMatch = 0;

	for (const auto& location : locations) {
		std::cout << "DEBUG: Checking location: " << location.path << " against path: " << path << std::endl;
		if (path.find(location.path) == 0 && location.path.length() > longestMatch) {
			matchedLocation = &location;
			longestMatch = location.path.length();
			std::cout << "DEBUG: Matched location: " << location.path << std::endl;
		}
	}

	if (!matchedLocation) {
		std::cout << "DEBUG: No specific location found, allowing GET by default" << std::endl;
		// No specific location found, allow GET by default
		return method == "GET";
	}

	std::cout << "DEBUG: Using location: " << matchedLocation->path << " with allowedMethods: " << matchedLocation->allowedMethods << std::endl;

	// Check if method is allowed in this location
	int methodFlag = 0;
	if (method == "GET") methodFlag = static_cast<int>(HttpMethod::GET);
	else if (method == "POST") methodFlag = static_cast<int>(HttpMethod::POST);
	else if (method == "DELETE") methodFlag = static_cast<int>(HttpMethod::DELETE);
	else if (method == "PUT") methodFlag = static_cast<int>(HttpMethod::PUT);
	else if (method == "HEAD") methodFlag = static_cast<int>(HttpMethod::HEAD);
	else {
		std::cout << "DEBUG: Unknown method: " << method << std::endl;
		return false; // Unknown method
	}

	bool allowed = (matchedLocation->allowedMethods & methodFlag) != 0;
	std::cout << "DEBUG: Method " << method << " (flag: " << methodFlag << ") allowed: " << (allowed ? "YES" : "NO") << std::endl;

	return allowed;
}

// Check if the POST body size is within limits
bool ClientHandler::isBodySizeValid() {
	// Extract Content-Length header
	size_t headerStart = _buffer.find("Content-Length: ");
	if (headerStart == std::string::npos) {
		return true; // No Content-Length header, assume valid
	}

	headerStart += 16; // Move past "Content-Length: "
	size_t headerEnd = _buffer.find("\r\n", headerStart);
	if (headerEnd == std::string::npos) {
		return true; // Malformed header, let it pass
	}

	std::string contentLengthStr = _buffer.substr(headerStart, headerEnd - headerStart);

	try {
		size_t contentLength = std::stoul(contentLengthStr);
		size_t maxBodySize = _owner.getClientBodySize();

		std::cout << "DEBUG: Content-Length: " << contentLength << ", Max allowed: " << maxBodySize << std::endl;

		return contentLength <= maxBodySize;
	} catch (const std::exception& e) {
		std::cout << "DEBUG: Error parsing Content-Length: " << e.what() << std::endl;
		return true; // Error parsing, let it pass
	}
}

// Get custom error page path for given status code
std::string ClientHandler::getCustomErrorPage(int statusCode) {
	const std::map<int, std::string>& errorPages = _owner.getErrorPages();
	auto it = errorPages.find(statusCode);
	if (it != errorPages.end()) {
		return it->second;
	}
	return ""; // No custom error page found
}

// Find appropriate index file for a directory
std::string ClientHandler::findIndexFile(const std::string& path) {
	// Get server configuration
	const std::vector<Location>& locations = _owner.getLocations();

	// Find matching location
	const Location* matchedLocation = nullptr;
	size_t longestMatch = 0;

	for (const auto& location : locations) {
		if (path.find(location.path) == 0 && location.path.length() > longestMatch) {
			matchedLocation = &location;
			longestMatch = location.path.length();
		}
	}

	std::string indexFile = "index.html"; // Default
	if (matchedLocation && !matchedLocation->index.empty()) {
		// Parse the first index file from the space-separated list
		std::string indexFiles = matchedLocation->index;

		// Remove trailing semicolon if present
		if (!indexFiles.empty() && indexFiles.back() == ';') {
			indexFiles.pop_back();
		}

		// Find first space or use entire string
		size_t spacePos = indexFiles.find(' ');
		if (spacePos != std::string::npos) {
			indexFile = indexFiles.substr(0, spacePos);
		} else {
			indexFile = indexFiles;
		}

		// Trim any remaining whitespace
		while (!indexFile.empty() && (indexFile.front() == ' ' || indexFile.front() == '\t')) {
			indexFile.erase(0, 1);
		}
		while (!indexFile.empty() && (indexFile.back() == ' ' || indexFile.back() == '\t')) {
			indexFile.pop_back();
		}
	}

	return indexFile;
}
