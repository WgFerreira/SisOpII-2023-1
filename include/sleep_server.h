#ifndef _SERVER_H
#define _SERVER_H

uint64_t now();
uint64_t millis_since(u_int64_t time);

void end_all_threads_safely();

#endif
