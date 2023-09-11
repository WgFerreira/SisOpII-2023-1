#ifndef _STATION_TABLE_H
#define _STATION_TABLE_H

#include <map>
#include <list>
#include <iostream>
#include <cstring>
#include <ifaddrs.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <iomanip>
#include <mutex>

#include "sleep_server.h"
#include "station.h"
  
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

    struct station_table_serial serialize();
    std::list<Station> getValues(unsigned int pid);
    bool has(std::string key);
    
    void insert(std::string key, Station item);
    void remove(std::string key);
    void replace(std::string key, Station item);
    void update(std::string key, StationStatus new_status, StationType new_type);
    void retry(std::string key);
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


#endif
