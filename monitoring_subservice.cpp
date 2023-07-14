#include <iostream>
#include <string>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>

#include "include/monitoring_subservice.h"
#include "include/sleep_server.h"

void *monitoring::server (Station* station) 
{
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        std::cout << "ERROR opening socket : monitor" << std::endl;
    
    struct sockaddr_in addr, recv_addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(MONITOR_PORT);
    memset(&(addr.sin_zero), 0, 8);

    while (true) 
    {
        struct hostent *server;
        if (hostTable.ipAddress.length() > 0) 
        {
            std::cout << hostTable.ipAddress << std::endl;
            server = gethostbyname(hostTable.ipAddress.c_str());
            if (server == NULL)
                std::cerr << "ERROR, no such host : monitor" << std::endl;

            addr.sin_addr = *((struct in_addr *) server->h_addr);
        
            struct packet buffer;
            buffer.type = DATA_PACKET;
            buffer.seqn = 0;
            buffer.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch() ).count();
            char* payload = "sleep status request\0";
            buffer.length = strnlen(payload, 255);
            strncpy((char *) buffer._payload, payload, buffer.length);

            socklen_t length = sizeof(struct sockaddr_in);
            int n = sendto(sockfd, &buffer, sizeof(buffer), 0, (struct sockaddr *) &recv_addr, length);
            std::cout << n << "/" << sizeof(buffer) << std::endl;

            sleep(10);
        }

    }
    
    close(sockfd);
}

void *monitoring::client (Station* station) {
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        std::cerr << "ERROR opening socket : monitor" << std::endl;

    struct sockaddr_in recv_addr, send_addr;
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(MONITOR_PORT);
    recv_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(recv_addr.sin_zero), 0, 8);

    if (bind(sockfd, (struct sockaddr *) &recv_addr, sizeof(struct sockaddr)) < 0)
        std::cerr << "ERROR on binding : monitor" << std::endl;

    struct packet recv_data;
    socklen_t send_addr_len = sizeof(struct sockaddr_in);

    while(true)
    {
        int n = recvfrom(sockfd, &recv_data, sizeof(struct packet), 0, (struct sockaddr *) &send_addr, &send_addr_len);
        if (n < 0)
            std::cerr << "ERROR on recvfrom : monitor" << std::endl;

        std::cout << recv_data.type << std::endl;
    }
    close(sockfd);
}
