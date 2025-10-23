#include "PostRequestHandler.hpp"
#include "IpPort.hpp"

void	PostRequestHandler::handlePostRequest(ClientPtr &client, const std::string &path)
{
	std::cout << "DEBUG: handlePostRequest() called with path: " << path << std::endl;
	if (!_bodyProcessingInitialized)
	{
		_bodyProcessingInitialized = true;
		client->setState(ClientState::GETTING_BODY);
		if (client->_chunked)
		{
			_readingChunkSize = true;
			_parsingChunkTrailers = false;
			_chunkedFinished = false;
		}
		else
		{
			_bodyBytesExpected = client->_contentLen;
		}
	}

	BodyReadStatus status = client->_chunked ? getChunkedBody(client) : getContentLengthBody(client);
	bool	isBodyFinished;
	if (client->_fileType == FileType::CGI_SCRIPT)
	{
		processPostCgi(client, status);
		return;
	}
	else
	{
		if (client->_contentType.find(CONTENT_TYPE_MULTIPART) != std::string::npos)
		{
			isBodyFinished = getMultiPart(client);
			if (isBodyFinished)
				writeBodyPart(client);
		}
		else if (client->_contentType.find(CONTENT_TYPE_APP_FORM) != std::string::npos)
			isBodyFinished = getFormPart(client);

		if (status == BodyReadStatus::NEED_MORE || !isBodyFinished)
			return;
		if (status == BodyReadStatus::COMPLETE && !isBodyFinished)
			THROW_HTTP(400, "No more content and part not finished");
	}
	resetBodyState();
	std::cout << "DEBUG: File uploaded successfully to: " << client->_resolvedPath + _uploadFilename << std::endl;
	std::string	addrPort = client->getIpPort().getAddrPort();
	std::string	port = addrPort.substr(addrPort.find(":") + 1);
	client->_redirectedUrl = LOCALHOST_URL + port + "/" + client->getHttpPath() + ".html";
	_ipPort.generateResponse(client, "", 303);
}

void	PostRequestHandler::writeBodyPart(ClientPtr &client)
{
	std::string uploadPath = composeUploadPath(client);
	std::ofstream out(uploadPath.c_str(), std::ios::binary | std::ios::trunc);
	if (!out.good())
		THROW_HTTP(500, "Couldn't open upload file for writing");
	if (!_decodedBuffer.empty())
		out.write(_decodedBuffer.data(), static_cast<std::streamsize>(_decodedBuffer.size()));
	out.close();
	_decodedBuffer.clear();
}

BodyReadStatus	PostRequestHandler::getContentLengthBody(ClientPtr &client)
{
	if (_bodyBytesExpected == 0)
		return BodyReadStatus::COMPLETE;

	while (!client->getBuffer().empty() && _bodyBytesReceived < _bodyBytesExpected)
	{
		size_t	remaining = _bodyBytesExpected - _bodyBytesReceived;
		size_t	toCopy = std::min(remaining, client->getBuffer().size());
		size_t	serverMax = client->getOwnerServer()->getClientBodySize();
		if (_bodyBytesReceived + toCopy > serverMax)
			THROW_HTTP(413, "Content too large");
		_bodyBuffer.append(client->getBuffer(), 0, toCopy);
		client->getBuffer().erase(0, toCopy);
		_bodyBytesReceived += toCopy;
	}

	if (_bodyBytesReceived < _bodyBytesExpected)
		return BodyReadStatus::NEED_MORE;

	return BodyReadStatus::COMPLETE;
}

BodyReadStatus	PostRequestHandler::getChunkedBody(ClientPtr &client)
{
	while (true)
	{
		if (_readingChunkSize)
		{
			size_t lineEnd = client->getBuffer().find("\r\n");
			if (lineEnd == std::string::npos)
			{
				if (client->getBuffer().size() > MAX_CHUNK_SIZE)
					THROW_HTTP(413, "Content too large");
				return BodyReadStatus::NEED_MORE;
			}
			std::string sizeLine = client->getBuffer().substr(0, lineEnd);
			client->getBuffer().erase(0, lineEnd + 2);
			if (sizeLine.empty())
				THROW_HTTP(400, "Chunk size line is empty");
			unsigned long chunkSize = 0;
			std::istringstream iss(sizeLine);
			iss >> std::hex >> chunkSize;
			if (iss.fail())
				THROW_HTTP(400, "Bad request");
			size_t	serverMax = client->getOwnerServer()->getClientBodySize();
			if (chunkSize > MAX_CHUNK_SIZE || _bodyBytesReceived + chunkSize > serverMax)
				THROW_HTTP(413, "Content too large");
			_currentChunkSize = static_cast<size_t>(chunkSize);
			_currentChunkRead = 0;
			_readingChunkSize = false;
			if (_currentChunkSize == 0)
				_parsingChunkTrailers = true;
			continue;
		}

		if (_parsingChunkTrailers)
		{
			if (client->getBuffer().size() >= 2 && client->getBuffer().substr(0, 2) == "\r\n")
			{
				client->getBuffer().erase(0, 2);
				_chunkedFinished = true;
				_parsingChunkTrailers = false;
				return BodyReadStatus::COMPLETE;
			}
			size_t trailerEnd = client->getBuffer().find("\r\n\r\n");
			if (trailerEnd == std::string::npos)
			{
				if (client->getBuffer().size() > MAX_CHUNK_SIZE)
					THROW_HTTP(413, "Content too large");
				return BodyReadStatus::NEED_MORE;
			}
			client->getBuffer().erase(0, trailerEnd + 4);
			_chunkedFinished = true;
			_parsingChunkTrailers = false;
			return BodyReadStatus::COMPLETE;
		}

		if (_currentChunkSize > 0)
		{
			if (client->getBuffer().empty())
				return BodyReadStatus::NEED_MORE;
			size_t remaining = _currentChunkSize - _currentChunkRead;
			size_t toCopy = std::min(remaining, client->getBuffer().size());
			_bodyBuffer.append(client->getBuffer(), 0, toCopy);
			client->getBuffer().erase(0, toCopy);
			_currentChunkRead += toCopy;
			_bodyBytesReceived += toCopy;
			if (_currentChunkRead < _currentChunkSize)
				return BodyReadStatus::NEED_MORE;
		}

		if (client->getBuffer().size() < 2)
			return BodyReadStatus::NEED_MORE;
		if (client->getBuffer()[0] != '\r' || client->getBuffer()[1] != '\n')
			THROW_HTTP(400, "Bad request");
		client->getBuffer().erase(0, 2);

		if (_parsingChunkTrailers)
			continue;

		_readingChunkSize = true;
		_currentChunkSize = 0;
		_currentChunkRead = 0;
	}

	return BodyReadStatus::NEED_MORE;
}

bool	PostRequestHandler::extractFilename(ClientPtr &client, std::string &dashBoundary)
{
	size_t bpos = _bodyBuffer.find(dashBoundary);
	if (bpos == std::string::npos)
	{
		size_t boundSize = dashBoundary.size();
		if (_bodyBuffer.size() > boundSize)
			_bodyBuffer.erase(0, _bodyBuffer.size() - boundSize);
		return false;
	}
	_bodyBuffer.erase(0, bpos + dashBoundary.size());
	if (_bodyBuffer.compare(0, 2, "\r\n") == 0)
		_bodyBuffer.erase(0, 2);

	size_t headersEnd = _bodyBuffer.find("\r\n\r\n");
	if (headersEnd == std::string::npos)
	{
		if (_bodyBuffer.size() > MAX_CHUNK_SIZE)
			THROW_HTTP(413, "Content too large");
		return false;
	}
	std::string			headers = _bodyBuffer.substr(0, headersEnd);
	std::istringstream	iss(headers);
	std::string			line;
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
		if (name == "Content-Disposition")
		{
			size_t fnPos = value.find("filename=");
			if (fnPos != std::string::npos)
			{
				size_t q1 = value.find('"', fnPos);
				size_t q2 = (q1 == std::string::npos) ? std::string::npos : value.find('"', q1 + 1);
				if (q1 != std::string::npos && q2 != std::string::npos && q2 > q1)
					_uploadFilename = value.substr(q1 + 1, q2 - q1 - 1);
			}
		}
	}
	if (_uploadFilename.empty())
		return false;
	_bodyBuffer.erase(0, headersEnd + 4);
	return false;
}

std::string	PostRequestHandler::composeUploadPath(ClientPtr &client)
{
	if (client->_resolvedPath.back() != '/')
		client->_resolvedPath += "/";
	return client->_resolvedPath + _uploadFilename;
}

void	PostRequestHandler::getLastBoundary(ClientPtr &client, std::string &boundaryMarker)
{
	if (_bodyBuffer.compare(0, boundaryMarker.size(), boundaryMarker) != 0)
		THROW_HTTP(400, "Body boundary missing");
	_bodyBuffer.erase(0, boundaryMarker.size());
	if (_bodyBuffer.compare(0, 2, "--") == 0)
		_bodyBuffer.erase(0, 2);
	if (_bodyBuffer.compare(0, 2, "\r\n") == 0)
		_bodyBuffer.erase(0, 2);
}

bool	PostRequestHandler::getMultiPart(ClientPtr &client)
{
	std::string dashBoundary = "--" + client->_multipartBoundary;
	std::string boundaryMarker = "\r\n" + dashBoundary;
	if (_uploadFilename.empty())
	{
		extractFilename(client, dashBoundary);
		if (_uploadFilename.empty())
			return false;
		if (_uploadFilename.find("#") != std::string::npos ||_uploadFilename.find(" ") != std::string::npos )
			THROW_HTTP(400, "Unsupported symbol");
	}
	size_t markerPos = _bodyBuffer.find(boundaryMarker);
	if (markerPos == std::string::npos)
	{
		size_t tail = boundaryMarker.size();
		if (_bodyBuffer.size() > tail)
		{
			size_t toAppend = _bodyBuffer.size() - tail;
			_decodedBuffer.append(_bodyBuffer.data(), toAppend);
			_bodyBuffer.erase(0, toAppend);
		}
		return false;
	}
	if (markerPos > 0)
	{
		_decodedBuffer.append(_bodyBuffer.data(), markerPos);
	}
	_bodyBuffer.erase(0, markerPos);
	getLastBoundary(client, boundaryMarker);
	return true;
}

std::string	PostRequestHandler::getParam(std::string body, std::string key)
{
	std::string	needle = key + "=";
	size_t	pos = body.find(needle);
	if (pos == std::string::npos)
		return std::string();
	pos += needle.size();
	size_t amp = body.find('&', pos);
	if (amp == std::string::npos)
		return body.substr(pos);
	return body.substr(pos, amp - pos);
}

bool	PostRequestHandler::getFormPart(ClientPtr &client)
{
	bool bodyComplete = (!client->_chunked
						&& _bodyBytesReceived >= _bodyBytesExpected)
						|| (client->_chunked && _chunkedFinished);
	if (!bodyComplete)
		return false;

	std::string	&body = _bodyBuffer;
	std::string	login = getParam(body, "login");
	std::string	pass = getParam(body, "password");
	if (!login.empty() || !pass.empty())
	{
		std::cout << "DEBUG: Received credentials login='" << login << "' password length=" << pass.size() << std::endl;
		return true;
	}

	return true;
}

void	PostRequestHandler::processPostCgi(ClientPtr &client, BodyReadStatus status)
{
	if (client->_fileFd == -1)
	{
		char tmpl[] = "/tmp/webserv_cgi_XXXXXX";
		client->_fileFd = mkstemp(tmpl);
		if (client->_fileFd == -1)
			THROW_HTTP(500, "mkstemp failed for CGI body temp file");
		unlink(tmpl);
	}

	if (!_bodyBuffer.empty())
	{
		size_t	len = write(client->_fileFd, _bodyBuffer.data(), _bodyBuffer.size());
		if (len < 0 || len != _bodyBuffer.size())
			THROW_HTTP(500, "Failed writing to CGI temp body file");
		client->_fileSize += len;
		_bodyBuffer.clear();
	}

	if (status == BodyReadStatus::NEED_MORE)
		return;

	if (lseek(client->_fileFd, 0, SEEK_SET) == -1)
		THROW_HTTP(500, "Failed to rewind CGI temp body file");

	if (!client->_cgi.init())
		THROW_HTTP(500, "Failed to start CGI process");
	client->setState(ClientState::WRITING_CGI_INPUT);
}

void	PostRequestHandler::resetBodyState()
{
	_uploadFilename.clear();
	_bodyBuffer.shrink_to_fit();
	_bodyBuffer.clear();
	_bodyBuffer.shrink_to_fit();
	_decodedBuffer.clear();
	_decodedBuffer.shrink_to_fit();
	_bodyBytesExpected = 0;
	_bodyBytesReceived = 0;
	_bodyProcessingInitialized = false;
	_currentChunkSize = 0;
	_currentChunkRead = 0;
	_readingChunkSize = true;
	_parsingChunkTrailers = false;
	_chunkedFinished = false;
}

// Constructors + Destructor

PostRequestHandler::PostRequestHandler(IpPort &owner)
	: _ipPort(owner)
{
	resetBodyState();
}

PostRequestHandler::~PostRequestHandler()
{}

