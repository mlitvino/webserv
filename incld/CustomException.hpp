#pragma once

#include <stdexcept>
#include <string>

#define BUFF_SIZE 128
#define THROW(msg) throw CustomException(__FILE__, __LINE__, msg, "")
#define THROW_ERRNO(msg) throw CustomException(__FILE__, __LINE__, msg, strerror(errno))

class CustomException : public std::exception
{
	private:
		char	_buf[BUFF_SIZE];
	public:
		CustomException(const char* file, int line, const char* msg, const char *strerror) noexcept
		{
			std::snprintf(_buf, sizeof(_buf), "%s:%d %s:%s", file, line, msg ? msg : "", strerror);
		}
		const char* what() const noexcept override
		{
			return _buf;
		}
};
