#include <iostream>
#include <unistd.h>
#include <string.h>
#include <semaphore>
#include <iterator>
#include <list>
#include <cmath>

#include "include/management_subservice.h"
#include "include/sleep_server.h"

using namespace std;
using namespace management;

void *management::manage(Station* station, ManagementQueue *queue, StationTable *table) 
{
  while(station->status != EXITING) 
  {
    if (table->has_update)
    {
      table->mutex_read.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      table->mutex_read.lock();
      table->has_update = false;
    }

    if (!queue->manage_queue.empty() && queue->mutex_manage.try_lock())
    {
      if (table->mutex_write.try_lock())
      {
        struct station_op_data op_data = queue->manage_queue.front();
        queue->manage_queue.pop_front();
        queue->mutex_manage.unlock();

        table->has_update = true;
        table->clock += 1;
        switch (op_data.operation)
        {
        case INSERT:
          table->table.insert(std::pair<std::string,Station>(op_data.key, op_data.station));
          table->table[op_data.key].last_update = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch() ).count();
          break;
        case DELETE:
          table->table.erase(op_data.key);
          break;
        case UPDATE_STATUS:
          table->table[op_data.key].status = op_data.new_status;
          table->table[op_data.key].last_update = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch() ).count();
          break;
        default:
        }
        
        table->mutex_write.unlock();
      }
      queue->mutex_manage.unlock();
    }
  }
  
  if (station->debug)
    std::cout << "saindo management" << std::endl;
}


struct management::station_table_serial &StationTable::serialize()
{
  unsigned long size = this->table.size();
  struct management::station_table_serial serialized[ceil(100.0)];

  serialized.clock = this->clock;
  serialized.count = this->table.size();
  serialized.table = new (struct station_serial[serialized.count]);

  return serialized;
}

void StationTable::deserialize(StationTable *table, station_table_serial serialized)
{
  table->clock = serialized.clock;
  table->table.clear();
  for (unsigned int i = 0; i < serialized.count; i++)
  {
    auto station = Station::deserialize(serialized.table[i]);

    table->table.insert(std::pair<std::string,Station>(station.macAddress, station));
  }
}


