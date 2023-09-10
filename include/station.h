#ifndef _STATION_H
#define _STATION_H

#include <iostream>
#include <string>
#include <cstring>
#include <semaphore>
#include <mutex>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <linux/if.h>

#include "input_parser.h"

#define MAC_ADDRESS_MAX 18

enum StationType : uint8_t 
{
    MANAGER,
    PARTICIPANT,
    CANDIDATE
};

enum StationStatus : uint8_t 
{
    AWAKEN,
    ELECTING,
    WAITING_ELECTION,
    ASLEEP,
    EXITING
};

/**
 * Representa a estação atual
*/
class Station
{
private:
    unsigned int pid = 0;
    unsigned int table_clock;

    StationType type = PARTICIPANT;
    Station *manager;

    std::string interface;

    StationStatus status = AWAKEN;
    std::string macAddress;
    std::string ipAddress;
    std::string hostname;

    u_int64_t last_leader_search; // last time the bully algorithm called for a leader
    int leader_search_retries; 

    u_int64_t last_update;
    u_int64_t last_update_request;
    u_int64_t update_request_retries;

    u_int64_t election_timeout = 500;
    u_int64_t monitor_interval = 1000;
    
    std::mutex mutex;

    void findInterfaceName();
    void findMacAddress();
    
public:
    bool debug = false;

    Station();
    Station(struct station_serial serialized);

    void init(InputParser *args);
    void printStation();

    struct station_serial serialize();
    void deserialize(struct station_serial serialized);

    struct sockaddr_in getSocketAddress(int port);
    in_addr_t getAddress();
    
    auto atomic_get(auto &&callback);
    void atomic_set(auto &&callback);

    unsigned int  getPid();
    void          SetPid(unsigned int pid);
    StationType GetType();
    void        SetType(StationType type);
    Station *GetManager();
    void    SetManager(Station *manager);
    StationStatus GetStatus();
    void          SetStatus(StationStatus status);
};

/**
 * Struct para enviar no pacote
*/
struct station_serial
{
    unsigned int pid;
    unsigned int table_clock;
    char hostname[HOST_NAME_MAX];
    char ipAddress[INET_ADDRSTRLEN];
    char macAddress[MAC_ADDRESS_MAX];
    StationStatus status;
    StationType type;
};

#endif
