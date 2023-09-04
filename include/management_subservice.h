#ifndef _MANAGEMENT_H
#define _MANAGEMENT_H

#include "datagram_subservice.h"
#include <map>
#include <list>

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
    Station station;
    StationStatus new_status;
  };

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
    }

    struct station_table_serial &serialize();
    void deserialize(StationTable *table, struct station_table_serial serialized);
    std::list<Station> getValues(unsigned int pid);
    bool has(std::string key);
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
    std::mutex mutex_manage;
    std::list<struct station_op_data> manage_queue;
  };

  /**
   * Realiza as operações na tabela de estações
   * 
  */
  void *manage(Station* station, ManagementQueue *queue, StationTable *table, datagram::DatagramQueue *dgram_queue);

};

#endif
