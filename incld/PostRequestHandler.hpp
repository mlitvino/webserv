#pragma once

#include "webserv.hpp"
#include "Client.hpp"

class IpPort;

enum class BodyReadStatus
{
	NEED_MORE,
	COMPLETE,
	ERROR
};

class PostRequestHandler
{
	private:
		IpPort &_ipPort;

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


};
