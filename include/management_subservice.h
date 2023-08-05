#ifndef _MANAGEMENT_H
#define _MANAGEMENT_H

#include "sleep_server.h"

namespace management {
  
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

  class StationTable
  {
  public:
    std::mutex mutex_write;
    std::mutex mutex_read;
    bool has_update;
    std::map<std::string,Station> table;
  };

  class ManagementQueue
  {
  public:
    std::mutex mutex_manage;
    std::list<struct station_op_data> manage_queue;
  };

  void *manage(Station* station, ManagementQueue *queue, StationTable *table);

};

#endif
