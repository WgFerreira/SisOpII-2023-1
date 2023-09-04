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

void *management::manage(Station* station, ManagementQueue *queue, StationTable *table, datagram::DatagramQueue *datagram_queue) 
{
  table->mutex_read.lock();
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
          break;
        }
        
        table->mutex_write.unlock();
      }
      queue->mutex_manage.unlock();
    }
  }
  
  if (station->debug)
    std::cout << "saindo management" << std::endl;
  return 0;
}

std::list<Station> management::StationTable::getValues(unsigned int pid) 
{
  std::list<Station> list;

  this->mutex_write.lock();
  for (auto &p : this->table)
    list.push_back(p.second);
  this->mutex_write.unlock();

  list.remove_if([&](Station &station) { return station.getPid() <= pid; });
  return list;
}

bool management::StationTable::has(std::string key) 
{
  this->mutex_write.lock();
  bool found_key = this->table.find(key) != this->table.end();
  this->mutex_write.unlock();
  return found_key;
}


struct management::station_table_serial &StationTable::serialize()
{
  // unsigned long count = this->table.size();
  // unsigned long size = (int)ceil(count/100);

  // struct management::station_table_serial serialized[size];

  // for (int i = 0; i < size; i++)
  // {
  //   struct management::station_table_serial t;

  //   t.clock = this->clock;
  //   t.count = this->table.size();
  //   t.table = new (struct station_serial[t.count]);
  // }
  


  // return serialized;
}

void StationTable::deserialize(StationTable *table, station_table_serial serialized)
{
  // table->clock = serialized.clock;
  // table->table.clear();
  // for (unsigned int i = 0; i < serialized.count; i++)
  // {
  //   auto station = Station::deserialize(serialized.table[i]);

  //   table->table.insert(std::pair<std::string,Station>(station.macAddress, station));
  // }
}


