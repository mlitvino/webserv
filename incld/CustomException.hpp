#pragma once

#include <stdexcept>
#include <string>
#include <cstdio>
#include <cerrno>
#include <cstring>

#define EXCEPT_BUFF_SIZE 128
#define THROW(msg) throw CustomException(__FILE__, __LINE__, msg, "")
#define THROW_ERRNO(msg) throw CustomException(__FILE__, __LINE__, msg, strerror(errno))

class CustomException : public std::exception {
private:
	std::string _message;

public:
	CustomException(const char* file, int line, const char* msg, const char* strerror) noexcept {
		char buffer[EXCEPT_BUFF_SIZE];
		std::snprintf(buffer, sizeof(buffer), "%s:%d %s: %s", 
					file, line, msg ? msg : "", strerror);
		_message = buffer;
	}
	
	const char* what() const noexcept override {
		return _message.c_str();
	}
};
