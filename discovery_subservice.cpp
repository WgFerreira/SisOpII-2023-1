#include <iostream>
#include <cstring>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "include/discovery_subservice.h"
#include "include/sleep_server.h"

void *discovery::server (Station* station) {
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
        std::cerr << "ERROR opening socket" << std::endl;

    struct sockaddr_in addr, recv;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(addr.sin_zero), 0, 8);    

    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr)) < 0) 
        std::cerr << "ERROR on binding" << std::endl;

    struct packet recv_data;
    socklen_t clilen = sizeof(struct sockaddr_in);

    while (true)
    {
        int n = recvfrom(sockfd, (struct packet *) &recv_data, sizeof(struct packet), 0, (struct sockaddr *) &recv, &clilen);
        if (n < 0) 
            std::cerr << "ERROR on recvfrom" << std::endl;
            
        char ip_address[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(recv.sin_addr), ip_address, INET_ADDRSTRLEN);
        std::cout << "Received a datagram from " << ip_address << ": " << recv_data._payload << std::endl;
        std::cout << recv_data.station.macAddress << std::endl;
        
        struct packet buffer;
        buffer.type = DATA_PACKET;
        buffer.seqn = 0;
        buffer.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch() ).count();
        char* payload = "I'm the leader\0";
        buffer.length = strnlen(payload, 255);
        strncpy((char*) buffer._payload, payload, buffer.length);
        buffer.station = *station;

        /* send to socket */
        n = sendto(sockfd, (struct packet *) &buffer, sizeof(buffer), 0,(struct sockaddr *) &recv, clilen);
        if (n  < 0) 
            std::cerr << "ERROR on sendto" << std::endl;
    }

    close(sockfd);

    return 0;
}

void *discovery::client (Station* station) {
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        std::cout << "ERROR opening socket" << std::endl;

    int broadcastEnable=1;
    int ret=setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
    if (ret < 0)
        std::cout << "ERROR option broadcast" << std::endl;
    
    struct sockaddr_in addr, recv;
    addr.sin_family = AF_INET;     
    addr.sin_port = htons(PORT);    
    addr.sin_addr.s_addr = INADDR_BROADCAST;
    memset(&(addr.sin_zero), 0, 8);

    struct packet buffer;
    buffer.type = DATA_PACKET;
    buffer.seqn = 0;
    buffer.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch() ).count();
    char* payload = "sleep service discovery\0";
    buffer.length = strnlen(payload, 255);
    strncpy((char*) buffer._payload, payload, buffer.length);
    buffer.station = *station;

    int n = sendto(sockfd, (struct packet *) &buffer, sizeof(buffer), 0, (const struct sockaddr *) &addr, sizeof(struct sockaddr_in));
    if (n < 0) 
        std::cout << "ERROR sendto" << std::endl;
        
    socklen_t length = sizeof(struct sockaddr_in);
    n = recvfrom(sockfd, (struct packet *) &buffer, sizeof(buffer), 0, (struct sockaddr *) &recv, &length);
    if (n < 0)
        std::cout << "ERROR recvfrom" << std::endl;

    std::cout << "Got an ack: " << buffer._payload << std::endl;
    std::cout << buffer.station.macAddress << std::endl;
    
    close(sockfd);

    return 0;
}

