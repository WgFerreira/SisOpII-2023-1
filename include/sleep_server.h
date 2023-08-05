#ifndef _SERVER_H
#define _SERVER_H

#include <string>
#include <limits.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <semaphore>
#include <mutex>
#include <map>

#define MAC_ADDRESS_MAX 18

#define MONITOR_INTERVAL 100

enum StationType : uint8_t 
{
    MANAGER,
    PARTICIPANT
};

enum StationStatus : uint8_t 
{
    AWAKEN,
    ASLEEP,
    EXITING
};

/**
 * Representa a estação atual
*/
class Station 
{
private:
    StationType type = PARTICIPANT;
    std::string interface;

    void findInterfaceName();
    void findMacAddress();
    
public:
    StationStatus status = AWAKEN;
    Station *manager;
    std::string macAddress;
    std::string ipAddress;
    std::string hostname;
    uint64_t last_update;
    bool debug = false;

    Station()
    {   // pega o time_t atual
        this->last_update = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch() ).count();
    }

    void init(std::string arg);
    void printStation();
    struct station_serial serialize();
    static Station deserialize(struct station_serial serialized);
    struct sockaddr_in getSocketAddress(int port);

    Station* getManager() { return this->manager; }
    void setManager(Station* manager) { this->manager = manager; }
    StationType getType() { return this->type; }
};

/**
 * Struct para enviar no pacote
*/
struct station_serial
{
    char hostname[HOST_NAME_MAX];
    char ipAddress[INET_ADDRSTRLEN];
    char macAddress[MAC_ADDRESS_MAX];
    StationStatus status;
};

enum ManagerOperation: uint16_t
{
    INSERT,
    UPDATE_STATUS,
    DELETE,
    NONE
};

struct station_op_data
{
    ManagerOperation operation;
    std::string key;
    Station station;
    StationStatus new_status;
};

struct semaphores
{
    std::mutex mutex_manager;
    std::mutex mutex_buffer;
    std::mutex mutex_write;
    std::mutex mutex_read;
};

class StationTable
{
public:
    bool has_update = true;
    struct station_op_data buffer;
    std::map<std::string,Station> table;
};


#endif
