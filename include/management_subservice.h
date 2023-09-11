#ifndef _MANAGEMENT_H
#define _MANAGEMENT_H

#include "datagram_subservice.h"
#include <map>
#include <list>

#include "sleep_server.h"

namespace management {
  
  class StationTable
  {
  public:
    unsigned long clock;
    std::mutex mutex_write;
    std::mutex mutex_read;
    bool has_update;
    std::map<std::string,Station> table;

    StationTable()
    {   
      this->clock = 0;
      this->has_update = false;
      // this->mutex_read.lock();
    }

    struct station_table_serial &serialize();
    void deserialize(StationTable *table, struct station_table_serial serialized);
    std::list<Station> getValues(unsigned int pid);
    bool has(std::string key);
    
    void insert(std::string key, Station item);
    void remove(std::string key);
    void update(std::string key, Station item);
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

  /**
   * Realiza as operações na tabela de estações
   * 
  */
  void *manage(Station* station, OperationQueue *manage_queue, StationTable *table, MessageQueue *send_queue);

};

#endif
