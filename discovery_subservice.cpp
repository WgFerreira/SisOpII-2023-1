#include <iostream>
#include <cstring>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "include/discovery_subservice.h"
#include "include/sleep_server.h"

void *discovery::server (Station* station, StationTable* table, struct semaphores *sem) 
{
    int sockfd = open_socket();

    struct sockaddr_in sock_addr = any_address(DISCOVERY_PORT); 

    if (bind(sockfd, (struct sockaddr *) &sock_addr, sizeof(struct sockaddr)) < 0) 
        std::cerr << "ERROR on binding : discovery" << std::endl;

    while (station->status != EXITING)
    {
        struct sockaddr_in client_addr;
        struct packet client_data;
        socklen_t client_addr_len = sizeof(struct sockaddr_in);

        int n = recvfrom(sockfd, &client_data, sizeof(struct packet), 0, (struct sockaddr *) &client_addr, &client_addr_len);
        if (n > 0 && validate_packet(&client_data, client_data.timestamp))
        {
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_data.station.ipAddress, INET_ADDRSTRLEN);
            Station participant = Station::deserialize(client_data.station);

            if (client_data.type == SLEEP_SERVICE_DISCOVERY || client_data.type == SLEEP_SERVICE_EXITING)
            {
                sem->mutex_buffer.lock();

                if (client_data.type == SLEEP_SERVICE_DISCOVERY)
                    table->buffer.operation = INSERT;
                else if (client_data.type == SLEEP_SERVICE_EXITING)
                    table->buffer.operation = DELETE;

                table->buffer.key = participant.macAddress;
                table->buffer.station = participant;
                
                sem->mutex_write.unlock();

                if (station->debug)
                {
                    std::cout << "Received a discovery packet from " << participant.ipAddress << ": " << client_data._payload << std::endl;
                    std::cout << participant.hostname << " " << participant.macAddress << std::endl;
                }

                struct packet data = create_packet(SLEEP_DISCOVERY_RESPONSE, 0, "I'm the leader");
                data.station = station->serialize();

                n = sendto(sockfd, &data, sizeof(data), 0,(struct sockaddr *) &client_addr, client_addr_len);
                if (n < 0) 
                    std::cerr << "ERROR on sendto : discovery" << std::endl;
            }
        }
    }

    struct sockaddr_in broadcast = broadcast_address(DISCOVERY_PORT);
    
    struct packet data = create_packet(SLEEP_SERVICE_EXITING, 0, "Bye!");
    data.station = station->serialize();
    
    int n = sendto(sockfd, &data, sizeof(data), 0, (const struct sockaddr *) &broadcast, sizeof(struct sockaddr_in));
    if (n < 0) 
        std::cout << "ERROR sendto exit : discovery" << std::endl;

    close(sockfd);
    if (station->debug)
        std::cout << "saindo discovery" << std::endl;
    return 0;
}

void *discovery::client (Station* station, StationTable* table, struct semaphores *sem) 
{
    int sockfd = open_socket();
    
    struct sockaddr_in sock_addr = any_address(DISCOVERY_PORT); 

    if (bind(sockfd, (struct sockaddr *) &sock_addr, sizeof(struct sockaddr)) < 0) 
        std::cerr << "ERROR on binding : discovery" << std::endl;

    while (station->status != EXITING)
    {
        /* Se não tem Manager envia um sleep discovery em broadcast */
        if (station->getManager() == NULL)
        {
            struct sockaddr_in sock_addr = broadcast_address(DISCOVERY_PORT);

            struct packet data = create_packet(SLEEP_SERVICE_DISCOVERY, 0, "sleep service discovery");
            data.station = station->serialize();
            
            int n = sendto(sockfd, &data, sizeof(data), 0, (const struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in));
            if (n < 0) 
                std::cout << "ERROR sendto : discovery" << std::endl;

            struct packet received_data;
            struct sockaddr_in server_addr;
            socklen_t server_addr_len = sizeof(struct sockaddr_in);
          
            int size = recvfrom(sockfd, &received_data, sizeof(struct packet), 0, (struct sockaddr *) &server_addr, &server_addr_len);
            if (size > 0 && validate_packet(&received_data, data.timestamp))
            {
                /* Se for um pacote de descoberta, é a resposta do manager */
                if (received_data.type == SLEEP_DISCOVERY_RESPONSE)
                {
                    inet_ntop(AF_INET, &(server_addr.sin_addr), received_data.station.ipAddress, INET_ADDRSTRLEN);
                    Station manager = Station::deserialize(received_data.station);
                    sem->mutex_manager.lock();
                    station->setManager(&manager);
                    table->has_update = true;
                    sem->mutex_manager.unlock();

                    if (station->debug)
                        std::cout << "Got an ack discovery packet from " << manager.macAddress << " " << manager.ipAddress << ": " << received_data._payload << std::endl;
                }
            }
        }
        
        if (station->getManager() != NULL)
        {
            struct packet received_data;
            struct sockaddr_in server_addr = station->getManager()->getSocketAddress(DISCOVERY_PORT);
            socklen_t sender_addr_len = sizeof(struct sockaddr_in);
            
            int n = recvfrom(sockfd, &received_data, sizeof(struct packet), 0, (struct sockaddr *) &server_addr, &sender_addr_len);
            if (n > 0 && validate_packet(&received_data, received_data.timestamp)) 
            {
                /* Se for um pacote de exiting, não tem mais manager */
                if (received_data.type == SLEEP_SERVICE_EXITING)
                {
                    sem->mutex_manager.lock();
                    if (station->manager)
                    {
                        //delete station->manager;
                        station->manager = NULL;
                        table->has_update = true;
                    }
                    sem->mutex_manager.unlock();

                    if (station->debug)
                        std::cout << "Got an exit packet from " << received_data.station.ipAddress << ": " << received_data._payload << std::endl;
                }
            }
        }
    }
    
    if (station->debug)
        std::cout << "Leaving Sleep Service" << std::endl;
    
    if (station->getManager() != NULL)
    {
        struct sockaddr_in server_addr = station->getManager()->getSocketAddress(DISCOVERY_PORT);
        
        struct packet data = create_packet(SLEEP_SERVICE_EXITING, 0, "Bye!");
        data.station = station->serialize();
        
        int n = sendto(sockfd, &data, sizeof(data), 0, (const struct sockaddr *) &server_addr, sizeof(struct sockaddr_in));
        if (n < 0) 
            std::cout << "ERROR sendto exit : discovery" << std::endl;
    }

    close(sockfd);
    if (station->debug)
        std::cout << "saindo discovery" << std::endl;
    return 0;
}
