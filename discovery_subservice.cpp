#include <iostream>
#include <cstring>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "include/discovery_subservice.h"
#include "include/sleep_server.h"

void *discovery::server (Station* station) 
{
    int sockfd = open_socket();

    struct sockaddr_in sock_addr = any_address(); 

    if (bind(sockfd, (struct sockaddr *) &sock_addr, sizeof(struct sockaddr)) < 0) 
        std::cerr << "ERROR on binding : discovery" << std::endl;

    while (station->status != EXITING)
    {
        struct sockaddr_in *client_addr;
        struct packet client_data;
        socklen_t client_addr_len = sizeof(struct sockaddr_in);

        int n = recvfrom(sockfd, &client_data, sizeof(struct packet), 0, (struct sockaddr *) client_addr, &client_addr_len);
        if (n > 0)
        {
            if (client_data.type == SLEEP_SERVICE_DISCOVERY)
            {
                inet_ntop(AF_INET, &(client_addr->sin_addr), client_data.station.ipAddress, INET_ADDRSTRLEN);

                // if (participant.status == AWAKEN) Add to host Table
                // if (participant.status == EXITING) Remove from host table
                Station participant = Station::deserialize(client_data.station);
                hostTable = participant;

                std::cout << "Received a datagram from " << participant.ipAddress << ": " << client_data._payload << std::endl;
                std::cout << participant.hostname << " " << participant.macAddress << std::endl;

                struct packet data = create_packet(SLEEP_SERVICE_DISCOVERY, 0, "I'm the leader");
                data.station = station->serialize();

                n = sendto(sockfd, &data, sizeof(data), 0,(struct sockaddr *) &client_addr, client_addr_len);
                if (n  < 0) 
                    std::cerr << "ERROR on sendto : discovery" << std::endl;
            }
        }
    }

    struct sockaddr_in broadcast = broadcast_address();
    
    struct packet data = create_packet(SLEEP_SERVICE_EXITING, 0, "Bye!");
    data.station = station->serialize();
    
    int n = sendto(sockfd, &data, sizeof(data), 0, (const struct sockaddr *) &broadcast, sizeof(struct sockaddr_in));
    if (n < 0) 
        std::cout << "ERROR sendto exit : discovery" << std::endl;

    close(sockfd);
    return 0;
}

void *discovery::client (Station* station) 
{
    int sockfd = open_socket();

    while (station->status != EXITING)
    {
        /* Se não tem Manager envia um sleep discovery em broadcast */
        if (station->getManager() == NULL)
        {
            struct sockaddr_in sock_addr = broadcast_address();

            struct packet data = create_packet(SLEEP_SERVICE_DISCOVERY, 0, "sleep service discovery");
            data.station = station->serialize();
            
            int n = sendto(sockfd, &data, sizeof(data), 0, (const struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in));
            if (n < 0) 
                std::cout << "ERROR sendto : discovery" << std::endl;

            struct packet received_data;
            struct sockaddr_in server_addr;
            socklen_t server_addr_len = sizeof(struct sockaddr_in);

            int size = recv_retry(sockfd, &received_data, sizeof(struct packet), &server_addr, &server_addr_len);
            if (size > 0 )
            {
                /* Se for um pacote de descoberta, é a resposta do manager */
                if (received_data.type == SLEEP_SERVICE_DISCOVERY)
                {
                    inet_ntop(AF_INET, &(server_addr.sin_addr), received_data.station.ipAddress, INET_ADDRSTRLEN);
                    
                    Station manager = Station::deserialize(received_data.station);
                    station->setManager(&manager);
                }
            }
        }
        
        if (station->getManager() != NULL)
        {
            struct packet received_data;
            struct sockaddr_in server_addr = station->getManager()->getSocketAddress();
            socklen_t sender_addr_len = sizeof(struct sockaddr_in);
            
            int n = recvfrom(sockfd, &received_data, sizeof(struct packet), 0, (struct sockaddr *) &server_addr, &sender_addr_len);
            if (n > 0) 
            {
                /* Se for um pacote de descoberta, é a resposta do manager */
                if (received_data.type == SLEEP_SERVICE_DISCOVERY)
                {
                    inet_ntop(AF_INET, &(server_addr.sin_addr), received_data.station.ipAddress, INET_ADDRSTRLEN);
                    
                    Station manager = Station::deserialize(received_data.station);
                    station->setManager(&manager);
                }
                /* Se for um pacote de exiting, não tem mais manager */
                else if (received_data.type == SLEEP_SERVICE_EXITING)
                    station->setManager(NULL);
            }
        }
    }
    
    if (station->getManager() == NULL)
    {
        struct sockaddr_in server_addr = station->getManager()->getSocketAddress();
        
        struct packet data = create_packet(SLEEP_SERVICE_EXITING, 0, "Bye!");
        data.station = station->serialize();
        
        int n = sendto(sockfd, &data, sizeof(data), 0, (const struct sockaddr *) &server_addr, sizeof(struct sockaddr_in));
        if (n < 0) 
            std::cout << "ERROR sendto exit : discovery" << std::endl;
    }

    close(sockfd);
    return 0;
}

