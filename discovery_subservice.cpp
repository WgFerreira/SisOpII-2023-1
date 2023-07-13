#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "include/discovery_subservice.h"
#include "include/sleep_server.h"

void *discovery::server (Station* station) {
    int sockfd, n;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    char buf[256];
        
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
        std::cerr << "ERROR opening socket" << std::endl;

    int broadcastEnable=1;
    int ret=setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(serv_addr.sin_zero), 0, 8);    
     
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
        std::cerr << "ERROR on binding" << std::endl;
    
    clilen = sizeof(struct sockaddr_in);
    
    while (1) {
        /* receive from socket */
        n = recvfrom(sockfd, buf, 256, 0, (struct sockaddr *) &cli_addr, &clilen);
        if (n < 0) 
            std::cerr << "ERROR on recvfrom" << std::endl;
        std::cout << "Received a datagram: " << buf << std::endl;
        
        /* send to socket */
        n = sendto(sockfd, "Got your message\n", 17, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
        if (n  < 0) 
            std::cerr << "ERROR on sendto" << std::endl;
    }

    close(sockfd);
    return 0;
}

void *discovery::client (Station* station) {
    int sockfd, n;
    unsigned int length;
    struct sockaddr_in serv_addr, from;
    struct hostent *server;
    char buffer[256];
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        std::cout << "ERROR opening socket" << std::endl;
        
    int broadcastEnable=1;
    int ret=setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
    
    serv_addr.sin_family = AF_INET;     
    serv_addr.sin_port = htons(PORT);    
    serv_addr.sin_addr.s_addr = INADDR_BROADCAST;
    memset(&(serv_addr.sin_zero), 0, 8);  

    std::cout << "Enter the message: " << std::endl;
    memset(buffer, 0, 256);
    fgets(buffer, 256, stdin);

    n = sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
    if (n < 0) 
        std::cout << "ERROR sendto" << std::endl;
    
    length = sizeof(struct sockaddr_in);
    n = recvfrom(sockfd, buffer, 256, 0, (struct sockaddr *) &from, &length);
    if (n < 0)
        std::cout << "ERROR recvfrom" << std::endl;

    std::cout << "Got an ack: " << buffer << std::endl;
    
    close(sockfd);

    return 0;
}

