#pragma once

#include <stdexcept>

#define EXCEPT_BUFF_SIZE 128
#define THROW_CHILD(msg) throw ChildFailedException(__FILE__, __LINE__, msg, strerror(errno))

class ChildFailedException : public std::exception
{
	private:
		char	_buf[EXCEPT_BUFF_SIZE];
	public:
		ChildFailedException(const char* file, int line, const char* msg, const char *strerror) noexcept
		{
			std::snprintf(_buf, sizeof(_buf), "%s:%d %s: %s", file, line, msg ? msg : "", strerror);
		}
		const char* what() const noexcept override
		{
			return _buf;
		}
};
