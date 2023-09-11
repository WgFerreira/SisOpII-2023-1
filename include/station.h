#ifndef _STATION_H
#define _STATION_H

#include <string>
#include <mutex>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>
#include "input_parser.h"
#include "sleep_server.h"

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

extern std::mutex mutex_no_manager;

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
  bool debug = false;

  Station()
  {   // pega o time_t atual
    this->last_update = now();
    this->last_leader_search = 0;
  }

  void init(InputParser *args);
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



#endif
