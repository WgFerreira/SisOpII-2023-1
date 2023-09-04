#ifndef _SERVER_H
#define _SERVER_H

#include <string>
#include <limits.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <semaphore>
#include <mutex>
#include <list>

#define MAC_ADDRESS_MAX 18

#define MONITOR_INTERVAL 100

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
    StationType type = PARTICIPANT;
    Station *manager;
    std::string interface;

    void findInterfaceName();
    void findMacAddress();
    
public:
  pid_t pid;
    StationStatus status = AWAKEN;
    std::string macAddress;
    std::string ipAddress;
    std::string hostname;
    u_int64_t last_leader_search; // last time the bully algorithm called for a leader
    int leader_search_retries; // last time the bully algorithm called for a leader
    u_int64_t last_update;
    bool debug = false;

    Station()
    {   // pega o time_t atual
        this->last_update = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        this->last_leader_search = 0;
    }

    void init();
    void printStation();
    struct station_serial serialize();
    static Station deserialize(struct station_serial serialized);
    struct sockaddr_in getSocketAddress(int port);
    in_addr_t getAddress();

    Station* getManager() { return this->manager; }
    void setManager(Station* manager) { this->manager = manager; }
    StationType getType() { return this->type; }
    void setType(StationType type) { this->type = type; }
    unsigned int getPid() { return this->pid; }
};

/**
 * Struct para enviar no pacote
*/
struct station_serial
{
    unsigned int pid;
    char hostname[HOST_NAME_MAX];
    char ipAddress[INET_ADDRSTRLEN];
    char macAddress[MAC_ADDRESS_MAX];
    StationStatus status;
    StationType type;
};


// ---------

uint64_t now();
uint64_t millis_since(u_int64_t time);

#endif
