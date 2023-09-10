// #include <iostream>
// #include <unistd.h>
// #include <string.h>
// #include <semaphore>
// #include <iterator>
// #include <list>
// #include <cmath>

#include "include/management_subservice.h"

using namespace std;
using namespace management;

void *management::manage(Station* station, ManagementQueue *queue, StationTable *table) 
{
  while(station->GetStatus() != EXITING) 
  {
    if (!queue->manage_queue.empty())
    {
      if (station->debug)
        std::cout << "management: processando fila de operações na tabela" << std::endl;

      struct station_op_data op_data = queue->atomic_pop();

      switch (op_data.operation)
      {
      case INSERT:
        if (station->debug)
          std::cout << "management: inserindo nova estação" << std::endl;
        table->push(op_data.key, op_data.station);
        break;

      case DELETE:
        if (station->debug)
          std::cout << "management: removendo uma estação se existir" << std::endl;
        table->remove(op_data.key);
        break;

      case UPDATE_STATUS:
        if (station->debug)
          std::cout << "management: atualizando status de uma estação se existir" << std::endl;
        table->update(op_data.key, op_data.new_status);
        break;

      default:
        break;
      }
    }
    
    if (table->has_update)
    {
      table->mutex_read.unlock();
      if (station->debug)
        std::cout << "management: leitura da tabela liberada" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      table->mutex_read.lock();
      table->has_update = false;
    }
  }
  
  if (station->debug)
    std::cout << "saindo management" << std::endl;
  return 0;
}

std::list<StationTableItem> management::StationTable::getValues(unsigned int pid) 
{
  std::list<StationTableItem> list;

  this->mutex_write.lock();
  for (auto &p : this->table)
    list.push_back(p.second);
  this->mutex_write.unlock();

  list.remove_if([&](StationTableItem &station) { return station.pid <= pid; });
  return list;
}

bool management::StationTable::has(std::string key) 
{
  this->mutex_write.lock();
  bool found_key = this->table.find(key) != this->table.end();
  this->mutex_write.unlock();
  return found_key;
}

void management::StationTable::push(std::string key, StationTableItem item)
{
  this->mutex_write.lock();
  this->has_update = true;
  this->clock += 1;
  this->table.insert(std::pair<std::string,StationTableItem>(key, item));
  this->table[key].last_update = now();
  this->table[key].update_request_retries = 0;
  this->mutex_write.unlock();
}

void management::StationTable::remove(std::string key)
{
  if (this->has(key))
  {
    this->mutex_write.lock();
    this->has_update = true;
    this->clock += 1;
    this->table.erase(key);
    this->mutex_write.unlock();
  }
}

void management::StationTable::update(std::string key, StationStatus new_status)
{
  if (this->has(key))
  {
    this->mutex_write.lock();
    this->has_update = true;
    this->clock += 1;
    this->table[key].status = new_status;
    this->table[key].last_update = now();
    this->table[key].update_request_retries = 0;
    this->mutex_write.unlock();
  }
}

void management::ManagementQueue::atomic_push(station_op_data op_data)
{
  this->mutex_manage.lock();
  this->manage_queue.push_back(op_data);
  this->mutex_manage.unlock();
}

station_op_data management::ManagementQueue::atomic_pop() 
{
  this->mutex_manage.lock();
  station_op_data data = this->manage_queue.front();
  this->manage_queue.pop_front();
  this->mutex_manage.unlock();
  return data;
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


