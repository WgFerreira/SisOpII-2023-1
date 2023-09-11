#ifndef _STATION_H
#define _STATION_H

#include <string>
#include <mutex>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>
#include <functional>
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
extern std::mutex mutex_station;

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

  StationStatus status = AWAKEN;
  std::string macAddress;

  std::string ipAddress;
  std::string hostname;

  u_int64_t last_leader_search;
 // last time the bully algorithm called for a leader
  int leader_search_retries; 

  u_int64_t last_update;
  u_int64_t update_request_retries;

  u_int64_t election_timeout = 500;
  u_int64_t monitor_interval = 1000;

  void findInterfaceName();
  void findMacAddress();
    
public:
  bool debug = false;

  Station()
  {   // pega o time_t atual
    this->last_leader_search = 0;
  }

  void init(InputParser *args);
  void printStation();
  void print_interface();
  struct station_serial serialize();
  static Station deserialize(struct station_serial serialized);
  struct sockaddr_in getSocketAddress(int port);
  in_addr_t getAddress();

  /**
   * USAR GET E SET COMUM PARA AS ESTAÇÕES NA TABELA
  */
  unsigned int GetPid() const { return this->pid; }
  std::string GetMacAddress() const { return this->macAddress; }
  StationType GetType() const { return this->type; }
  void        SetType(StationType type) { this->type = type; }
  StationStatus GetStatus() const { return status; }
  void          SetStatus(StationStatus status) { this->status = status; }
  u_int64_t GetLast_leader_search() const { return last_leader_search; }
  void      SetLast_leader_search(u_int64_t last_leader_search) { this->last_leader_search = last_leader_search; }
  int   GetLeader_search_retries() const { return leader_search_retries; }  
  void  SetLeader_search_retries(int leader_search_retries) { this->leader_search_retries = leader_search_retries; }
  std::string GetHostname() const { return hostname; }
  u_int64_t GetUpdate_request_retries() const { return update_request_retries; }
  void      SetUpdate_request_retries(u_int64_t update_request_retries) { update_request_retries = update_request_retries; }
  void SetLast_update(u_int64_t last_update) { this->last_update = last_update; }
  
  /**
   * USAR GET E SET atomico PARA A ESTAÇÃO ATUAL DO SISTEMA
  */
  auto atomic_get(auto &&callback);
  void atomic_set(std::function<void(Station *)> callback);

  StationType atomic_GetType();
  void        atomic_SetType(StationType type);
  Station *atomic_GetManager();
  void    atomic_SetManager(Station *manager);
  StationStatus atomic_GetStatus();
  void          atomic_SetStatus(StationStatus status);
  int   atomic_GetLeader_search_retries();
  void  atomic_SetLeader_search_retries(int leader_search_retries);
  u_int64_t atomic_GetLast_leader_search();
  void      atomic_SetLast_leader_search(u_int64_t last_leader_search);
  u_int64_t atomic_GetElection_timeout();
  void      atomic_SetElection_timeout(u_int64_t election_timeout);
  u_int64_t atomic_GetLast_update();
  void      atomic_SetLast_update(u_int64_t last_update);
  u_int64_t atomic_GetMonitor_interval();
  void      atomic_SetMonitor_interval(u_int64_t monitor_interval);
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
