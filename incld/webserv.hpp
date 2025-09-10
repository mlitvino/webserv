#pragma once

#include <iostream>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "CustomException.hpp"

#define IN_PORT "8080"
#define IN_DOMAIN "localhost"
#define QUEUE_SIZE 20
