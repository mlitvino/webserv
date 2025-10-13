#pragma once

#include <stdexcept>
#include <string>

#define EXCEPT_BUFF_SIZE 128
#define THROW_HTTP(statusCode, msg) throw HttpException((statusCode), __FILE__, __LINE__, (msg), "")
#define THROW_HTTP_ERRNO(statusCode, msg) throw HttpException((statusCode), __FILE__, __LINE__, (msg), strerror(errno))

class HttpException : public std::exception
{
	private:
		int		_statusCode;
		char	_buf[EXCEPT_BUFF_SIZE];
	public:
		HttpException(int statusCode, const char* file, int line, const char* msg, const char* strerr) noexcept
			: _statusCode(statusCode)
		{
			std::snprintf(_buf, sizeof(_buf), "%s:%d [%d] %s", file, line, statusCode, msg);
		}
		int	getStatusCode() const noexcept { return _statusCode; }
		const char* what() const noexcept override { return _buf; }
};
