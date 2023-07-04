#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "include/sleep_server.h"

void *discovery (Station* station) {
    std::cout << "descobrindo" << std::endl;

    if (station->type == MANAGER)
        std::cout << "sou uma estação manager" << std::endl;
    else
        std::cout << "sou só uma estação participante" << std::endl;

    return 0;
}
