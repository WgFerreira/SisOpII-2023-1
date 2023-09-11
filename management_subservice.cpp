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

void *management::manage(Station* station, OperationQueue *manage_queue, StationTable *table, MessageQueue *send_queue) 
{
  manage_queue->mutex_read.lock();
  while(station->status != EXITING) 
  {
    manage_queue->mutex_read.lock();
    while (!manage_queue->queue.empty())
    {
      if (station->debug)
        std::cout << "management: processando fila de operações na tabela" << std::endl;

      table_operation op_data = manage_queue->pop();

      switch (op_data.operation)
      {
      case INSERT:
        if (station->debug)
          std::cout << "management: inserindo nova estação" << std::endl;
        table->insert(op_data.key, op_data.station);
        break;

      case DELETE:
        if (station->debug)
          std::cout << "management: removendo uma estação se existir" << std::endl;
        table->remove(op_data.key);
        break;

      case UPDATE_STATUS:
        if (station->debug)
          std::cout << "management: atualizando status de uma estação se existir" << std::endl;
        table->update(op_data.key, op_data.station);
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
      // std::this_thread::sleep_for(std::chrono::milliseconds(100));
      // table->mutex_read.lock();
      // table->has_update = false;
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

void management::StationTable::insert(std::string key, Station item)
{
  this->mutex_write.lock();
  this->has_update = true;
  this->clock += 1;
  this->table.insert(std::pair<std::string,Station>(key, item));
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

void management::StationTable::update(std::string key, Station item)
{
  if (this->has(key))
  {
    this->mutex_write.lock();
    this->has_update = true;
    this->clock += 1;
    this->table[key].status = item.status;
    this->table[key].setType(item.getType());
    this->table[key].last_update = now();
    this->table[key].update_request_retries = 0;
    this->mutex_write.unlock();
  }
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


