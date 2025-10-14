#include "PostRequestHandler.hpp"
#include "IpPort.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

PostRequestHandler::PostRequestHandler(IpPort &owner)
	: _ipPort(owner)
{}

void	PostRequestHandler::handlePostRequest(ClientPtr &client, const std::string &path)
{
	std::cout << "DEBUG: handlePostRequest() called with path: " << path << std::endl;

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

	BodyReadStatus status = client->_chunked ? getChunkedBody(client) : getContentLengthBody(client);
	bool isBodyFinished = getMultiPart(client);

	if (status == BodyReadStatus::NEED_MORE || !isBodyFinished)
		return;
	if (status == BodyReadStatus::COMPLETE && !isBodyFinished)
		THROW_HTTP(400, "No more content and part not finished");

	client->resetBodyTracking();
	std::cout << "DEBUG: File uploaded successfully to: " << client->_resolvedPath + client->_uploadFilename << std::endl;
	_ipPort.generateResponse(client, "", 201);
}

BodyReadStatus	PostRequestHandler::getContentLengthBody(ClientPtr &client)
{
	if (client->_bodyBytesExpected == 0)
		return BodyReadStatus::COMPLETE;

	while (!client->_buffer.empty() && client->_bodyBytesReceived < client->_bodyBytesExpected)
	{
		size_t remaining = client->_bodyBytesExpected - client->_bodyBytesReceived;
		size_t toCopy = std::min(remaining, client->_buffer.size());
		if (client->_bodyBytesReceived + toCopy > kMaxRequestBodySize)
			THROW_HTTP(413, "Content too large");
		client->_bodyBuffer.append(client->_buffer, 0, toCopy);
		client->_buffer.erase(0, toCopy);
		client->_bodyBytesReceived += toCopy;
	}

	if (client->_bodyBytesReceived < client->_bodyBytesExpected)
		return BodyReadStatus::NEED_MORE;

	return BodyReadStatus::COMPLETE;
}

BodyReadStatus	PostRequestHandler::getChunkedBody(ClientPtr &client)
{
	while (true)
	{
		if (client->_readingChunkSize)
		{
			size_t lineEnd = client->_buffer.find("\r\n");
			if (lineEnd == std::string::npos)
			{
				if (client->_buffer.size() > kMaxChunkHeaderSize)
					THROW_HTTP(413, "Content too large");
				return BodyReadStatus::NEED_MORE;
			}
			std::string sizeLine = client->_buffer.substr(0, lineEnd);
			client->_buffer.erase(0, lineEnd + 2);
			if (sizeLine.empty())
				THROW_HTTP(400, "Chunk size line is empty");
			unsigned long chunkSize = 0;
			std::istringstream iss(sizeLine);
			iss >> std::hex >> client->_currentChunkSize;
			if (iss.fail())
				THROW_HTTP(400, "Bad request");
			if (chunkSize > kMaxChunkDataSize || client->_bodyBytesReceived + chunkSize > kMaxRequestBodySize)
				THROW_HTTP(413, "Content too large");
			client->_currentChunkSize = static_cast<size_t>(chunkSize);
			client->_currentChunkRead = 0;
			client->_readingChunkSize = false;
			if (client->_currentChunkSize == 0)
				client->_parsingChunkTrailers = true;
			continue;
		}

		if (client->_parsingChunkTrailers)
		{
			if (client->_buffer.size() >= 2 && client->_buffer.substr(0, 2) == "\r\n")
			{
				client->_buffer.erase(0, 2);
				client->_chunkedFinished = true;
				client->_parsingChunkTrailers = false;
				return BodyReadStatus::COMPLETE;
			}
			size_t trailerEnd = client->_buffer.find("\r\n\r\n");
			if (trailerEnd == std::string::npos)
			{
				if (client->_buffer.size() > kMaxTrailersSize)
					THROW_HTTP(413, "Content too large");
				return BodyReadStatus::NEED_MORE;
			}
			client->_buffer.erase(0, trailerEnd + 4);
			client->_chunkedFinished = true;
			client->_parsingChunkTrailers = false;
			return BodyReadStatus::COMPLETE;
		}

		if (client->_currentChunkSize > 0)
		{
			if (client->_buffer.empty())
				return BodyReadStatus::NEED_MORE;
			size_t remaining = client->_currentChunkSize - client->_currentChunkRead;
			size_t toCopy = std::min(remaining, client->_buffer.size());
			client->_bodyBuffer.append(client->_buffer, 0, toCopy);
			client->_buffer.erase(0, toCopy);
			client->_currentChunkRead += toCopy;
			client->_bodyBytesReceived += toCopy;
			if (client->_currentChunkRead < client->_currentChunkSize)
				return BodyReadStatus::NEED_MORE;
		}

		if (client->_buffer.size() < 2)
			return BodyReadStatus::NEED_MORE;
		if (client->_buffer[0] != '\r' || client->_buffer[1] != '\n')
			THROW_HTTP(400, "Bad request");
		client->_buffer.erase(0, 2);

		if (client->_parsingChunkTrailers)
			continue;

		client->_readingChunkSize = true;
		client->_currentChunkSize = 0;
		client->_currentChunkRead = 0;
	}

	return BodyReadStatus::NEED_MORE;
}

bool	PostRequestHandler::extractFilename(ClientPtr &client, std::string &dashBoundary)
{
	size_t bpos = client->_bodyBuffer.find(dashBoundary);
	if (bpos == std::string::npos)
	{
		size_t boundSize = dashBoundary.size();
		if (client->_bodyBuffer.size() > boundSize)
			client->_bodyBuffer.erase(0, client->_bodyBuffer.size() - boundSize);
		return false;
	}
	client->_bodyBuffer.erase(0, bpos + dashBoundary.size());
	if (client->_bodyBuffer.compare(0, 2, "\r\n") == 0)
		client->_bodyBuffer.erase(0, 2);

	size_t headersEnd = client->_bodyBuffer.find("\r\n\r\n");
	if (headersEnd == std::string::npos)
	{
		if (client->_bodyBuffer.size() > kMaxChunkHeaderSize)
			THROW_HTTP(413, "Content too large");
		return false;
	}
	std::string headers = client->_bodyBuffer.substr(0, headersEnd);
	std::istringstream iss(headers);
	std::string line;
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
					client->_uploadFilename = value.substr(q1 + 1, q2 - q1 - 1);
			}
		}
	}
	if (client->_uploadFilename.empty())
		return false;
	client->_bodyBuffer.erase(0, headersEnd + 4);
	return false;
}

std::string	PostRequestHandler::composeUploadPath(ClientPtr &client)
{
	if (client->_resolvedPath.back() != '/')
		client->_resolvedPath += "/";
	return client->_resolvedPath + client->_uploadFilename;
}

void	PostRequestHandler::writeBodyPart(ClientPtr &client, std::string &uploadPath, size_t tailSize)
{
	if (client->_bodyBuffer.size() > tailSize)
	{
		size_t toWrite = client->_bodyBuffer.size() - tailSize;
		std::ofstream out(uploadPath.c_str(), std::ios::binary | std::ios::app);
		if (!out.good())
			THROW_HTTP(500, "Coudl't open file");
		out.write(client->_bodyBuffer.data(), static_cast<std::streamsize>(toWrite));
		out.close();
		client->_bodyBuffer.erase(0, toWrite);
	}
}

void	PostRequestHandler::getLastBoundary(ClientPtr &client, std::string &boundaryMarker)
{
	if (client->_bodyBuffer.compare(0, boundaryMarker.size(), boundaryMarker) != 0)
		THROW_HTTP(400, "Body boundary missing");
	client->_bodyBuffer.erase(0, boundaryMarker.size());
	if (client->_bodyBuffer.compare(0, 2, "--") == 0)
		client->_bodyBuffer.erase(0, 2);
	if (client->_bodyBuffer.compare(0, 2, "\r\n") == 0)
		client->_bodyBuffer.erase(0, 2);
}

bool	PostRequestHandler::getMultiPart(ClientPtr &client)
{
	std::string dashBoundary = "--" + client->_multipartBoundary;
	std::string boundaryMarker = "\r\n" + dashBoundary;
	if (client->_uploadFilename.empty())
	{
		extractFilename(client, dashBoundary);
		if (client->_uploadFilename.empty())
			return false;
	}
	std::string uploadPath = composeUploadPath(client);

	size_t markerPos = client->_bodyBuffer.find(boundaryMarker);
	if (markerPos == std::string::npos)
	{
		size_t tail = boundaryMarker.size();
		writeBodyPart(client, uploadPath, tail);
		return false;
	}
	if (markerPos > 0)
	{
		std::ofstream out(uploadPath.c_str(), std::ios::binary | std::ios::app);
		if (!out.good())
			THROW_HTTP(500, "Failed to open file");
		out.write(client->_bodyBuffer.data(), static_cast<std::streamsize>(markerPos));
		out.close();
	}
	client->_bodyBuffer.erase(0, markerPos);
	getLastBoundary(client, boundaryMarker);
	return true;
}
