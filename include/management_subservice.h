#ifndef _MANAGEMENT_H
#define _MANAGEMENT_H

#include <iostream>
#include <semaphore>
// #include <unistd.h>
// #include <string.h>
// #include <semaphore>
// #include <iterator>
// #include <list>
// #include <cmath>
#include <map>
#include <list>

#include "station.h"
#include "sleep_server.h"

namespace management {
  
  enum ManagerOperation: uint16_t
  {
    INSERT,         /** key, station */
    UPDATE_STATUS,  /** key, new_status */
    DELETE,         /** key */
    NONE
  };

  struct station_op_data
  {
    ManagerOperation operation;
    std::string key;
    StationTableItem station;
    StationStatus new_status;
  };

  class StationTableItem
  {
  public:
    unsigned int pid = 0;
    unsigned int table_clock;

    StationType type = PARTICIPANT;

    StationStatus status = AWAKEN;
    std::string macAddress;
    std::string ipAddress;
    std::string hostname;

    u_int64_t last_update;
    u_int64_t last_update_request;
    u_int64_t update_request_retries;
  };

  class StationTable
  {
  public:
    unsigned long clock;
    std::mutex mutex_write;
    std::mutex mutex_read; // apenas para a interface
    bool has_update;
    std::map<std::string, StationTableItem> table;

    StationTable()
    {   
      this->clock = 0;
      this->has_update = false;
      this->mutex_read.lock();
    }

    struct station_table_serial &serialize();
    void deserialize(StationTable *table, struct station_table_serial serialized);
    std::list<StationTableItem> getValues(unsigned int pid);
    bool has(std::string key);

    void push(std::string key, StationTableItem item);
    void remove(std::string key);
    void update(std::string key, StationStatus new_status);
  };

  /**
   * Struct para enviar no pacote
  */
  struct station_table_serial
  {
    unsigned long clock;
    unsigned int count;
    struct station_serial table[100];
  };

  class ManagementQueue
  {
  public:
    std::mutex mutex_release;
    std::mutex mutex_manage;
    std::list<struct station_op_data> manage_queue;

    void atomic_push(station_op_data op_data);
    station_op_data atomic_pop(); 
    
  };

  /**
   * Realiza as operações na tabela de estações
   * 
  */
  void *manage(Station* station, ManagementQueue *queue, StationTable *table);

};

#endif
