#ifndef _SERVER_H
#define _SERVER_H

#include <string>
#include <limits.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <semaphore>
#include <mutex>
#include <list>

#include "input_parser.h"

uint64_t now();
uint64_t millis_since(u_int64_t time);

#endif
