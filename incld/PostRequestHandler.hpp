#pragma once

#include <memory>
#include <string>
#include <fstream>

#include "webserv.hpp"
#include "ChildFailedException.hpp"

enum class BodyReadStatus
{
	NEED_MORE,
	COMPLETE,
	ERROR
};

#define MAX_CHUNK_SIZE 1010241024

class PostRequestHandler
{
	private:
		IpPort			&_ipPort;

		std::string		_uploadFilename;
		std::string		_bodyBuffer;
		std::string		_decodedBuffer;
		std::ofstream	_uploadStream;
		std::string		_currentUploadPath;
		size_t			_bodyBytesExpected;
		size_t			_bodyBytesReceived;
		bool			_bodyProcessingInitialized;
		size_t			_currentChunkSize;
		size_t			_currentChunkRead;
		bool			_readingChunkSize;
		bool			_parsingChunkTrailers;
		bool			_chunkedFinished;

		BodyReadStatus	getContentLengthBody(ClientPtr &client);
		BodyReadStatus	getChunkedBody(ClientPtr &client);
		bool			getMultiPart(ClientPtr &client);
		std::string		getParam(std::string body, std::string key);

		bool			extractFilename(std::string &dashBoundary);
		std::string		composeUploadPath(ClientPtr &client);
		void			writeBodyPart(ClientPtr &client);
		void			getLastBoundary(std::string &boundaryMarker);
		void			processPostCgi(ClientPtr &client, BodyReadStatus status);
	public:
		PostRequestHandler(IpPort &owner);
		~PostRequestHandler();
		void			handlePostRequest(ClientPtr &client, const std::string &path);
		void			resetBodyState();
};
