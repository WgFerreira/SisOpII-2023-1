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
    int sockfd = open_socket();

    while (station->status != EXITING) 
    {
        smphAccessHostTable.acquire();
        for (auto &tupla : stations_table)
        {
            auto station = tupla.second;
            if (station.ipAddress.length() > 0) 
            {
                struct sockaddr_in sock_addr = station.getSocketAddress();
                
                struct packet data = create_packet(SLEEP_STATUS_REQUEST, 0, "sleep status request");
                data.station = station.serialize();

                int n = sendto(sockfd, &data, sizeof(data), 0, (struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in));
                if (n < 0)
                    std::cout << "ERROR sendto : monitor" << std::endl;
            }            
        }
        smphAccessHostTable.release();
        
        if (hostTable.ipAddress.length() > 0) 
        {
            struct sockaddr_in sock_addr = hostTable.getSocketAddress();
        
            struct packet data = create_packet(SLEEP_STATUS_REQUEST, 0, "sleep status request");
            data.station = hostTable.serialize();

            int n = sendto(sockfd, &data, sizeof(data), 0, (struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in));
            if (n < 0)
                std::cout << "ERROR sendto : monitor" << std::endl;
            
            struct packet received_data;
            struct sockaddr_in server_addr;
            socklen_t server_addr_len = sizeof(struct sockaddr_in);

            int size = recvfrom(sockfd, &received_data, sizeof(struct packet), 0, (struct sockaddr *) &server_addr, &server_addr_len);
            if (size > 0 && validate_packet(&received_data, data.timestamp))
            {
                inet_ntop(AF_INET, &(server_addr.sin_addr), received_data.station.ipAddress, INET_ADDRSTRLEN);

                Station participant = Station::deserialize(received_data.station);
                hostTable.status = participant.status;

                if (station->debug)
                    std::cout << "Got a sleep status packet from " << participant.ipAddress << ": " << received_data._payload << std::endl;
            }
            else
            {
                hostTable.status = ASLEEP;
                if (station->debug)
                    std::cout << "Didn't get a sleep status packet from " << hostTable.ipAddress << std::endl;
            }

            sleep(MONITOR_INTERVAL);
        }
    }
    
    close(sockfd);
    return 0;
}

void *monitoring::client (Station* station) 
{
    int sockfd = open_socket();

    struct sockaddr_in sock_addr = any_address(); 

    if (bind(sockfd, (struct sockaddr *) &sock_addr, sizeof(struct sockaddr)) < 0) 
        std::cerr << "ERROR on binding : monitor" << std::endl;

    while(station->status != EXITING)
    {
        if (station->getManager() != NULL)
        {
            struct sockaddr_in client_addr;
            struct packet client_data;
            socklen_t client_addr_len = sizeof(struct sockaddr_in);
            
            int size = recvfrom(sockfd, &client_data, sizeof(struct packet), 0, (struct sockaddr *) &client_addr, &client_addr_len);
            if (size > 0)
            {
                if (client_data.type == SLEEP_STATUS_REQUEST)
                {
                    char client_ip_addr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip_addr, INET_ADDRSTRLEN);
                    std::cout << "Got a sleep status request from " << client_ip_addr << ": " << client_data._payload << std::endl;

                    station->ipAddress = client_data.station.ipAddress;

                    struct packet data = create_packet(SLEEP_STATUS_REQUEST, 0, "AWAKEN");
                    data.station = station->serialize();

                    int n = sendto(sockfd, &data, sizeof(data), 0,(struct sockaddr *) &client_addr, client_addr_len);
                    if (n  < 0) 
                        std::cerr << "ERROR on sendto : monitor" << std::endl;
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
