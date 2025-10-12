#include "IpPort.hpp"
#include "Cgi.hpp"

void	IpPort::OpenSocket(addrinfo &hints, addrinfo *_servInfo)
{
	int	err;

	size_t		delim = _addrPort.find_last_of(":");
	std::string	host = _addrPort.substr(0, delim);
	std::string	port = _addrPort.substr(delim + 1, _addrPort.size() - delim);

	err = getaddrinfo(host.c_str(), port.c_str(), &hints, &_servInfo);
	if (err == -1)
		THROW(gai_strerror(err));

	_sockFd = socket(_servInfo->ai_family, _servInfo->ai_socktype, _servInfo->ai_protocol);
	if (_sockFd == -1)
		THROW_ERRNO("socket");

	int opt = 1;

	err = setsockopt(_sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (err == -1)
		THROW_ERRNO("setsockopt(SO_REUSEADDR)");

	err = bind(_sockFd, _servInfo->ai_addr, _servInfo->ai_addrlen);
	if (err == -1)
		THROW_ERRNO("bind");

	err = listen(_sockFd, QUEUE_SIZE);
	if (err == -1)
		THROW_ERRNO("listen");
}

void	IpPort::handleEpollEvent(epoll_event &ev, int epollFd, int eventFd)
{
	if (eventFd == getSockFd())
	{
		std::cout << "Accepting new connection..." << std::endl;
		acceptConnection(ev, epollFd, eventFd);
		std::cout << "Connection was accepted" << std::endl;
	}
	else
	{
		ClientPtr	client = (*_clientsMap.find(eventFd)).second;
		if (ev.events & EPOLLIN)
		{
			if (client->_state == ClientState::READING_REQUEST)
			{
				std::cout << "Accepting data from existing client..." << std::endl;
				if (!client->readRequest())
					return closeConnection(eventFd);
				parseRequest(ev, epollFd, eventFd);
				std::cout << "Accepting data is done" << std::endl;
			}
			else if (client->_state == ClientState::GETTING_BODY)
			{
				std::cout << "Continuing to read POST body..." << std::endl;
				if (!client->readRequest())
					return closeConnection(eventFd);
				handlePostRequest(client, client->_httpPath);
			}
		}
	}
}

void	IpPort::parseRequest(epoll_event &ev, int epollFd, int eventFd)
{
	ClientPtr	client = (*_clientsMap.find(eventFd)).second;

	if (client->_buffer.find("\r\n\r\n") == std::string::npos)
		return ;

	std::cout << "DEBUG: Buffer content: " << client->_buffer << std::endl;
	parseHeaders(client);
	std::cout << "Method: " << client->_httpMethod << ", Path: " << client->_httpPath << ", Version: " << client->_httpVersion << std::endl;
	assignServerToClient(client);

	bool valid = client->_ownerServer->areHeadersValid(client);
	if (!valid)
		return ;

	client->resetBodyTracking();

	size_t headersEnd = client->_buffer.find("\r\n\r\n");
	if (headersEnd != std::string::npos)
		client->_buffer.erase(0, headersEnd + 4);

	if (client->_httpMethod == "GET")
	{
		handleGetRequest(client, client->_httpPath);
	}
	else if (client->_httpMethod == "POST")
	{
		handlePostRequest(client, client->_httpPath);
	}
	else if (client->_httpMethod == "DELETE")
	{
		//handleDeleteRequest(client->_httpPath);
	}
}

void	IpPort::parseHeaders(ClientPtr &client)
{
	size_t headersEnd = client->_buffer.find("\r\n\r\n");
	if (headersEnd == std::string::npos)
		return;

	std::string			headers = client->_buffer.substr(0, headersEnd);
	std::istringstream	iss(headers);
	std::string			line;

	client->_contentLen = 0;
	client->_chunked = false;
	client->_keepAlive = false;
	client->_contentType.clear();
	client->_multipartBoundary.clear();

	std::getline(iss, line);
	size_t firstSpace = line.find(' ');
	size_t secondSpace = line.find(' ', firstSpace + 1);
	client->_httpMethod = line.substr(0, firstSpace);
	client->_httpPath = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
	client->_httpVersion = line.substr(secondSpace + 1);

	while (std::getline(iss, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;
		std::string name = line.substr(0, colon);
		std::string value = line.substr(colon + 1);

		while (!value.empty() && isspace(value.front()))
			value.erase(0, 1);
		while (!value.empty() && isspace(value.back()))
			value.pop_back();

		if (name == "Host")
		{
			client->_hostHeader = value;
		}
		else if (name == "Content-Length")
		{
			client->_contentLen = std::stoul(value);
		}
		else if (name == "Content-Type")
		{
			client->_contentType = value;
			if (value.find("multipart/form-data") != std::string::npos)
			{
				size_t	bpos = value.find("boundary=");
				if (bpos != std::string::npos)
				{
					size_t	valpos = bpos + strlen("boundary=");
					client->_multipartBoundary = value.substr(valpos);

					std::cout << "DEBUG: _miltipartBoundary " << client->_multipartBoundary << std::endl;
				}
			}
		}
		else if (name == "Transfer-Encoding")
		{
			if (value.find("chunked") != std::string::npos)
				client->_chunked = true;
		}
		else if (name == "Connection")
		{
			if (value.find("keep-alive") != std::string::npos)
				client->_keepAlive = true;
		}
	}
}

void IpPort::handleGetRequest(ClientPtr &client, const std::string& path)
{
	std::cout << "DEBUG: handleGetRequest() called with path: " << path << std::endl;

	std::string fullPath = client->_ownerServer->findFile(client, path);
	std::cout << "DEBUG: findFile returned: " << fullPath << std::endl;

	if (fullPath.empty())
	{
		std::cout << "DEBUG: File not found, generating 404 response" << std::endl;
		generateResponse(client, "", 404);
		return;
	}
	else
	{
		generateResponse(client, fullPath, 200);
	}

	std::cout << "DEBUG: Response buffer size after generation: " << client->_responseBuffer.size() << std::endl;
}

BodyReadStatus IpPort::getContentLengthBody(Client &client, int &errorStatus)
{
	if (client._bodyBytesExpected > kMaxRequestBodySize)
	{
		errorStatus = 413;
		return BodyReadStatus::ERROR;
	}

	if (client._bodyBytesExpected == 0)
		return BodyReadStatus::COMPLETE;

	while (!client._buffer.empty() && client._bodyBytesReceived < client._bodyBytesExpected)
	{
		size_t remaining = client._bodyBytesExpected - client._bodyBytesReceived;
		size_t toCopy = std::min(remaining, client._buffer.size());
		if (client._bodyBytesReceived + toCopy > kMaxRequestBodySize)
		{
			errorStatus = 413;
			return BodyReadStatus::ERROR;
		}
		client._bodyBuffer.append(client._buffer, 0, toCopy);
		client._buffer.erase(0, toCopy);
		client._bodyBytesReceived += toCopy;
	}

	if (client._bodyBytesReceived < client._bodyBytesExpected)
		return BodyReadStatus::NEED_MORE;

	return BodyReadStatus::COMPLETE;
}

BodyReadStatus IpPort::getChunkedBody(Client &client, int &errorStatus)
{
	while (true)
	{
		if (client._parsingChunkTrailers)
		{
			if (client._buffer.size() >= 2 && client._buffer.substr(0, 2) == "\r\n")
			{
				client._buffer.erase(0, 2);
				client._chunkedFinished = true;
				client._parsingChunkTrailers = false;
				return BodyReadStatus::COMPLETE;
			}
			size_t trailerEnd = client._buffer.find("\r\n\r\n");
			if (trailerEnd == std::string::npos)
			{
				if (client._buffer.size() > kMaxTrailersSize)
				{
					errorStatus = 400;
					return BodyReadStatus::ERROR;
				}
				return BodyReadStatus::NEED_MORE;
			}
			client._buffer.erase(0, trailerEnd + 4);
			client._chunkedFinished = true;
			client._parsingChunkTrailers = false;
			return BodyReadStatus::COMPLETE;
		}

		if (client._readingChunkSize)
		{
			size_t lineEnd = client._buffer.find("\r\n");
			if (lineEnd == std::string::npos)
			{
				if (client._buffer.size() > kMaxChunkHeaderSize)
				{
					errorStatus = 400;
					return BodyReadStatus::ERROR;
				}
				return BodyReadStatus::NEED_MORE;
			}
			std::string sizeLine = client._buffer.substr(0, lineEnd);
			client._buffer.erase(0, lineEnd + 2);
			size_t semicolon = sizeLine.find(';');
			if (semicolon != std::string::npos)
				sizeLine = sizeLine.substr(0, semicolon);
			if (sizeLine.empty())
			{
				errorStatus = 400;
				return BodyReadStatus::ERROR;
			}
			unsigned long chunkSize = 0;
			std::istringstream iss(sizeLine);
			iss >> std::hex >> chunkSize;
			if (iss.fail())
			{
				errorStatus = 400;
				return BodyReadStatus::ERROR;
			}
			if (chunkSize > kMaxChunkDataSize || client._bodyBytesReceived + chunkSize > kMaxRequestBodySize)
			{
				errorStatus = 413;
				return BodyReadStatus::ERROR;
			}
			client._currentChunkSize = static_cast<size_t>(chunkSize);
			client._currentChunkRead = 0;
			client._readingChunkSize = false;
			if (client._currentChunkSize == 0)
				client._parsingChunkTrailers = true;
			continue;
		}

		if (client._currentChunkSize > 0)
		{
			if (client._buffer.empty())
				return BodyReadStatus::NEED_MORE;
			size_t remaining = client._currentChunkSize - client._currentChunkRead;
			size_t toCopy = std::min(remaining, client._buffer.size());
			client._bodyBuffer.append(client._buffer, 0, toCopy);
			client._buffer.erase(0, toCopy);
			client._currentChunkRead += toCopy;
			client._bodyBytesReceived += toCopy;
			if (client._currentChunkRead < client._currentChunkSize)
				return BodyReadStatus::NEED_MORE;
		}

		if (client._buffer.size() < 2)
			return BodyReadStatus::NEED_MORE;
		if (client._buffer[0] != '\r' || client._buffer[1] != '\n')
		{
			errorStatus = 400;
			return BodyReadStatus::ERROR;
		}
		client._buffer.erase(0, 2);

		if (client._parsingChunkTrailers)
			continue;

		client._readingChunkSize = true;
		client._currentChunkSize = 0;
		client._currentChunkRead = 0;
	}

	return BodyReadStatus::NEED_MORE;
}

bool IpPort::drainMultipartFirstPartToFile(Client &client,
											 const std::string &boundary,
											 const std::string &uploadDir,
											 bool &finished,
											 int &errorStatus)
{
	finished = false;
	if (boundary.empty())
	{
		errorStatus = 400;
		return false;
	}

	const std::string dashBoundary = "--" + boundary;
	const std::string boundaryMarker = "\r\n" + dashBoundary;

	if (client._uploadFilename.empty())
	{
		size_t bpos = client._bodyBuffer.find(dashBoundary);
		if (bpos == std::string::npos)
		{
			size_t keep = dashBoundary.size();
			if (client._bodyBuffer.size() > keep)
				client._bodyBuffer.erase(0, client._bodyBuffer.size() - keep);
			return true;
		}
		client._bodyBuffer.erase(0, bpos + dashBoundary.size());
		if (client._bodyBuffer.compare(0, 2, "\r\n") == 0)
			client._bodyBuffer.erase(0, 2);

		size_t headersEnd = client._bodyBuffer.find("\r\n\r\n");
		if (headersEnd == std::string::npos)
		{
			if (client._bodyBuffer.size() > kMaxChunkHeaderSize)
			{
				errorStatus = 400;
				return false;
			}
			return true;
		}

		std::string headers = client._bodyBuffer.substr(0, headersEnd);
		std::istringstream iss(headers);
		std::string line;
		while (std::getline(iss, line))
		{
			if (!line.empty() && line.back() == '\r') line.pop_back();
			size_t colon = line.find(':');
			if (colon == std::string::npos) continue;
			std::string name = line.substr(0, colon);
			std::string value = line.substr(colon + 1);
			while (!value.empty() && isspace(value.front())) value.erase(0, 1);
			if (name == "Content-Disposition")
			{
				size_t fnPos = value.find("filename=");
				if (fnPos != std::string::npos)
				{
					size_t q1 = value.find('"', fnPos);
					size_t q2 = (q1 == std::string::npos) ? std::string::npos : value.find('"', q1 + 1);
					if (q1 != std::string::npos && q2 != std::string::npos && q2 > q1)
						client._uploadFilename = value.substr(q1 + 1, q2 - q1 - 1);
				}
			}
		}

		if (client._uploadFilename.empty())
		{
			errorStatus = 400;
			return false;
		}

		client._bodyBuffer.erase(0, headersEnd + 4);
	}

	if (client._uploadFilename.empty())
		return true;

	std::string cleanUploadDir = uploadDir;
	if (!cleanUploadDir.empty() && cleanUploadDir.back() != '/')
		cleanUploadDir += '/';
	if (!ensureDirExists("web/www/uploads") || !ensureDirExists(cleanUploadDir))
	{
		errorStatus = 500;
		return false;
	}
	std::string uploadPath = cleanUploadDir + client._uploadFilename;

	// Look for boundary marker in buffer
	size_t markerPos = client._bodyBuffer.find(boundaryMarker);
	if (markerPos == std::string::npos)
	{
		// Write everything except a tail large enough to match marker next time
		size_t tail = boundaryMarker.size();
		if (client._bodyBuffer.size() > tail)
		{
			size_t toWrite = client._bodyBuffer.size() - tail;
			std::ofstream out(uploadPath.c_str(), std::ios::binary | std::ios::app);
			if (!out.good()) { errorStatus = 500; return false; }
			out.write(client._bodyBuffer.data(), static_cast<std::streamsize>(toWrite));
			out.close();
			client._bodyBuffer.erase(0, toWrite);
		}
		return true; // need more data to find boundary
	}

	// Write data up to boundary start (markerPos points to CRLF before --boundary)
	if (markerPos > 0)
	{
		std::ofstream out(uploadPath.c_str(), std::ios::binary | std::ios::app);
		if (!out.good()) { errorStatus = 500; return false; }
		out.write(client._bodyBuffer.data(), static_cast<std::streamsize>(markerPos));
		out.close();
	}
	client._bodyBuffer.erase(0, markerPos);

	// Remove boundary marker itself
	if (client._bodyBuffer.compare(0, boundaryMarker.size(), boundaryMarker) != 0)
	{
		errorStatus = 400;
		return false;
	}
	client._bodyBuffer.erase(0, boundaryMarker.size());

	// Check for final boundary suffix "--" or move past CRLF
	if (client._bodyBuffer.compare(0, 2, "--") == 0)
	{
		client._bodyBuffer.erase(0, 2);
		finished = true; // final boundary -> done
	}
	if (client._bodyBuffer.compare(0, 2, "\r\n") == 0)
		client._bodyBuffer.erase(0, 2);

	// For single-part handling, encountering any boundary means we finished first part
	finished = true;
	return true;
}

bool IpPort::parseMultipartFirstPart(const std::string &body, const std::string &boundary, std::string &outFilename, std::string &outData)
{
	std::string dashBoundary = "--" + boundary;
	std::string endBoundary = dashBoundary + "--";
	size_t start = body.find(dashBoundary);
	if (start == std::string::npos) return false;
	start += dashBoundary.size();
	if (body.substr(start, 2) == "\r\n") start += 2;
	size_t partHeadersEnd = body.find("\r\n\r\n", start);
	if (partHeadersEnd == std::string::npos) return false;
	std::string partHeaders = body.substr(start, partHeadersEnd - start);

	// extract filename from Content-Disposition if present
	outFilename.clear();
	std::istringstream phs(partHeaders);
	std::string hline;
	while (std::getline(phs, hline))
	{
		if (!hline.empty() && hline.back() == '\r') hline.pop_back();
		size_t colon = hline.find(':');
		if (colon == std::string::npos) continue;
		std::string hname = hline.substr(0, colon);
		std::string hval = hline.substr(colon + 1);
		while (!hval.empty() && isspace(hval.front())) hval.erase(0, 1);
		if (hname == "Content-Disposition")
		{
			size_t fnPos = hval.find("filename=");
			if (fnPos != std::string::npos)
			{
				size_t quoteStart = hval.find('"', fnPos);
				size_t quoteEnd = std::string::npos;
				if (quoteStart != std::string::npos)
					quoteEnd = hval.find('"', quoteStart + 1);
				if (quoteStart != std::string::npos && quoteEnd != std::string::npos)
					outFilename = hval.substr(quoteStart + 1, quoteEnd - (quoteStart + 1));
			}
		}
	}

	size_t dataStart = partHeadersEnd + 4;
	size_t dataEnd = body.find("\r\n" + dashBoundary, dataStart);
	if (dataEnd == std::string::npos)
	{
		size_t endMarker = body.find(endBoundary, dataStart);
		if (endMarker != std::string::npos)
			dataEnd = (endMarker >= 2 ? endMarker - 2 : endMarker);
	}
	if (dataEnd == std::string::npos) dataEnd = body.size();
	outData = body.substr(dataStart, dataEnd - dataStart);
	return true;
}

bool IpPort::ensureDirExists(const std::string &dir)
{
	if (dir.empty())
		return false;
	std::string path = dir;

	while (path.size() > 1 && path.back() == '/') path.pop_back();

	struct stat st;
	if (stat(path.c_str(), &st) == 0)
		return S_ISDIR(st.st_mode);

	size_t pos = path.find_last_of('/');
	if (pos != std::string::npos)
	{
		std::string parent = path.substr(0, pos);
		if (!parent.empty() && parent != path)
		{
			if (!ensureDirExists(parent)) return false;
		}
	}

	if (mkdir(path.c_str(), 0755) != 0)
	{
		if (errno == EEXIST)
			return true;
		return false;
	}
	return true;
}

void	IpPort::handlePostRequest(ClientPtr &client, const std::string& path)
{
	std::cout << "DEBUG: handlePostRequest() called with path: " << path << std::endl;

	if (client->_contentType.find("multipart/form-data") == std::string::npos)
	{
		std::cout << "DEBUG: Unsupported Content-Type for POST: " << client->_contentType << std::endl;
		generateResponse(client, "", 405);
		return;
	}

	if (client->_multipartBoundary.empty())
	{
		std::cout << "DEBUG: Missing multipart boundary" << std::endl;
		generateResponse(client, "", 400);
		return;
	}

	if (!client->_bodyProcessingInitialized)
	{
		client->_bodyProcessingInitialized = true;
		client->_bodyBytesReceived = 0;
		client->_bodyBuffer.clear();
		client->_state = ClientState::GETTING_BODY;
		if (client->_chunked)
		{
			client->_readingChunkSize = true;
			client->_currentChunkSize = 0;
			client->_currentChunkRead = 0;
			client->_parsingChunkTrailers = false;
			client->_chunkedFinished = false;
		}
		else
		{
			client->_bodyBytesExpected = client->_contentLen;
		}
	}

	int errorStatus = 400;
	BodyReadStatus status = client->_chunked
		? getChunkedBody(*client, errorStatus)
		: getContentLengthBody(*client, errorStatus);

	if (status == BodyReadStatus::ERROR)
	{
		std::cout << "DEBUG: Error while reading request body, status " << errorStatus << std::endl;
		client->resetBodyTracking();
		generateResponse(client, "", errorStatus);
		return;
	}

	std::string cleanPath = path;
	if (!cleanPath.empty() && cleanPath[0] == '/')
		cleanPath = cleanPath.substr(1);
	std::string	uploadDir = std::string("web/www/uploads/") + cleanPath;
	bool partFinished = false;
	if (!drainMultipartFirstPartToFile(*client, client->_multipartBoundary, uploadDir, partFinished, errorStatus))
	{
		std::cout << "DEBUG: drainMultipartFirstPartToFile error status " << errorStatus << std::endl;
		client->resetBodyTracking();
		generateResponse(client, "", errorStatus);
		return;
	}

	if (status == BodyReadStatus::NEED_MORE || !partFinished)
		return;

	if (status == BodyReadStatus::COMPLETE && !partFinished)
	{
		client->resetBodyTracking();
		generateResponse(client, "", 400);
		return;
	}

	std::string finalPath = uploadDir;
	if (!finalPath.empty() && finalPath.back() != '/') finalPath += '/';
	finalPath += client->_uploadFilename;
	client->resetBodyTracking();
	std::cout << "DEBUG: File uploaded successfully to: " << finalPath << std::endl;
	generateResponse(client, "", 201);
	std::cout << "DEBUG: Response buffer size after generation: " << client->_responseBuffer.size() << std::endl;
}

void	IpPort::handleDeleteRequest(ClientPtr &client, const std::string& path)
{
	std::cout << "DEBUG: handleDeleteRequest() called with path: " << path << std::endl;

	std::string fullPath = client->_ownerServer->findFile(client, path);
	std::cout << "DEBUG: findFile returned for deletion: " << fullPath << std::endl;

	if (fullPath.empty())
	{
		std::cout << "DEBUG: File not found for deletion, generating 404 response" << std::endl;
		generateResponse(client, "", 404);
		return;
	}

	if (std::remove(fullPath.c_str()) == 0)
	{
		std::cout << "DEBUG: File deleted successfully, generating 204 response" << std::endl;
		generateResponse(client, "", 204);
	}
	else
	{
		std::cout << "DEBUG: Failed to delete file, generating 500 response" << std::endl;
		generateResponse(client, "", 500);
	}

	std::cout << "DEBUG: Response buffer size after generation: " << client->_responseBuffer.size() << std::endl;
}

void	IpPort::generateResponse(ClientPtr &client, std::string filePath, int statusCode)
{
	std::string	statusText;
	std::string	response;

	switch (statusCode)
	{
		case 200: statusText = "OK"; break;
		case 201: statusText = "Created"; break;
		case 204: statusText = "No Content"; break;
		case 404: statusText = "Not Found"; break;
		case 405: statusText = "Method Not Allowed"; break;
		case 413: statusText = "Payload Too Large"; break;
		case 500: statusText = "Internal Server Error"; break;
		default: statusText = "Unknown"; break;
	}

	if (statusCode != 200 && statusCode != 201)
		filePath = client->_ownerServer->getCustomErrorPage(statusCode);

	// For 204 No Content, we don't send a body
	if (statusCode == 204)
	{
		response = "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n";
		response += "Server: webserv/1.0\r\n";
		response += "Connection: close\r\n";
		response += "\r\n";

		client->_state = ClientState::SENDING_RESPONSE;
		client->_responseBuffer = std::move(response);
		return;
	}

	if (!filePath.empty())
	{
		std::cout << "DEBUG: filepath name -> " << filePath << std::endl;
		client->openFile(filePath);
		if (client->_fileFd < 0)
		{
			THROW_ERRNO("open");
			std::cout << "DEBUG: Coudln't open filePath" << std::endl;
			statusCode = 500;
			statusText = "Internal Server Error";
		}
	}
	else
	{
		std::cout << "DEBUG: filePath is empty" << std::endl;
	}

	// add MIME TYPE
	response = "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + std::to_string(client->_fileSize) + "\r\n";
	response += "Server: webserv/1.0\r\n";
	response += "Connection: close\r\n";
	response += "Cache-Control: no-cache\r\n";
	response += "\r\n";

	client->_state = ClientState::SENDING_RESPONSE;
	utils::changeEpollHandler(_handlersMap, client->_clientFd, client.get());
	client->_responseBuffer = std::move(response);
}

void	IpPort::assignServerToClient(ClientPtr &client)
{
	client->_ownerServer = _servers.front();

	std::string hostValue = client->_hostHeader;
	if (!hostValue.empty())
	{
		size_t colon = hostValue.find(':');
		if (colon != std::string::npos)
			hostValue = hostValue.substr(0, colon);

		for (auto &server : _servers)
		{
			if (server->getServerName() == hostValue)
			{
				client->_ownerServer = server;
				return ;
			}
		}
	}
}

// std::string	IpPort::getMimeType(const std::string& filePath)
// {
// 	if (filePath.empty())
// 		return "text/html";

// 	size_t dotPos = filePath.find_last_of('.');
// 	if (dotPos == std::string::npos)
// 		return "text/plain";

// 	std::string extension = filePath.substr(dotPos + 1);

// 	// Convert to lowercase
// 	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

// 	if (extension == "html" || extension == "htm")
// 		return "text/html";
// 	else if (extension == "css")
// 		return "text/css";
// 	else if (extension == "js")
// 		return "application/javascript";
// 	else if (extension == "json")
// 		return "application/json";
// 	else if (extension == "txt")
// 		return "text/plain";
// 	else if (extension == "png")
// 		return "image/png";
// 	else if (extension == "jpg" || extension == "jpeg")
// 		return "image/jpeg";
// 	else if (extension == "gif")
// 		return "image/gif";
// 	else if (extension == "ico")
// 		return "image/x-icon";
// 	else
// 		return "application/octet-stream";
// }

// void	IpPort::processCgi(ClientPtr &client)
// {
// 	try {
// 		Cgi cgi(*client);
// 		if (!cgi.start()) {
// 			generateResponse(client, "", 500);
// 			return;
// 		}
// 		client->_state = ClientState::CGI_READING_OUTPUT;
// 	}
// 	catch (const std::exception &e) {
// 		std::cerr << "CGI start exception: " << e.what() << std::endl;
// 		generateResponse(client, "", 500);
// 	}
// }

void	IpPort::acceptConnection(epoll_event &ev, int epollFd, int eventFd)
{
	int					err;
	epoll_event			newEv;
	int					clientFd;
	sockaddr_storage	clientAddr;
	socklen_t			clientAddrLen = sizeof(clientAddr);

	try
	{
		clientFd = accept(_sockFd, (sockaddr *)&clientAddr, &clientAddrLen);
		if (clientFd == -1)
			THROW_ERRNO("accept");
		utils::makeFdNonBlocking(clientFd);
		ClientPtr	newClient = std::make_shared<Client>(clientAddr, clientAddrLen, clientFd, *this);
		_clientsMap.emplace(clientFd, newClient);
		_handlersMap.emplace(clientFd, this);
		newEv.events = EPOLLIN | EPOLLOUT;
		newEv.data.fd = clientFd;
		err = epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &newEv);
		if (err)
		{
			_handlersMap.erase(clientFd);
			_clientsMap.erase(clientFd);
			close(clientFd);
			THROW_ERRNO("epoll_ctl");
		}
	}
	catch (const std::exception& e)
	{
		if (clientFd != -1)
		{
			_handlersMap.erase(clientFd);
			_clientsMap.erase(clientFd);
			close(clientFd);
		}
		std::cerr << "Exception:" << e.what() << std::endl;
	}
}

void	IpPort::closeConnection(int clientFd)
{
	std::cout << "Closing connection..." << std::endl;

	_handlersMap.erase(clientFd);
	_clientsMap.erase(clientFd);

	int err = epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, 0);
	if (err)
	{
		THROW("epoll_ctl(EPOLL_CTL_DEL)");
	}

	std::cout << "Closing connection is done" << std::endl;
}

// Getters + Setters

int	IpPort::getSockFd()
{
	return _sockFd;
}

void	IpPort::setAddrPort(std::string addrPort)
{
	_addrPort = addrPort;
}

// Constructors + Destructor

IpPort::~IpPort()
{
	if (_sockFd != -1)
		close(_sockFd);
}

IpPort::IpPort(Program &program)
	: _clientsMap{program._clientsMap}
	, _handlersMap{program._handlersMap}
	, _sockFd{-1}
	, _epollFd{program._epollFd}
{};
