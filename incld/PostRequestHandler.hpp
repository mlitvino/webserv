#pragma once

#include <memory>

class Client;
class IpPort;
using ClientPtr = std::shared_ptr<Client>;

enum class BodyReadStatus
{
	NEED_MORE,
	COMPLETE,
	ERROR
};

const size_t kMaxRequestBodySize = 50 * 1024 * 1024;
const size_t kMaxChunkDataSize = 10 * 1024 * 1024;
const size_t kMaxChunkHeaderSize = 8 * 1024;
const size_t kMaxTrailersSize = 8 * 1024;

class PostRequestHandler
{
	private:
		IpPort &_ipPort;

		std::string	_uploadFilename;
		std::string	_bodyBuffer;
		size_t		_bodyBytesExpected = 0;
		size_t		_bodyBytesReceived = 0;
		bool		_bodyProcessingInitialized = false;
		size_t		_currentChunkSize = 0;
		size_t		_currentChunkRead = 0;
		bool		_readingChunkSize = true;
		bool		_parsingChunkTrailers = false;
		bool		_chunkedFinished = false;



		BodyReadStatus	getContentLengthBody(ClientPtr &client);
		BodyReadStatus	getChunkedBody(ClientPtr &client);
		bool			getMultiPart(ClientPtr &client);
		bool			getFormPart(ClientPtr &client);
		std::string		getParam(std::string body, std::string key);

		bool			extractFilename(ClientPtr &client, std::string &dashBoundary);
		std::string		composeUploadPath(ClientPtr &client);
		void			writeBodyPart(ClientPtr &client, std::string &uploadPath, size_t tailSize);
		void			getLastBoundary(ClientPtr &client, std::string &boundaryMarker);
	public:
		explicit		PostRequestHandler(IpPort &owner);

		void			handlePostRequest(ClientPtr &client, const std::string &path);

		void			resetBodyState();
};
