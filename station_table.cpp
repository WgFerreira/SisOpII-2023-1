#include "include/station_table.h"
#include "include/station.h"

#include <iostream>
#include <cstring>
#include <ifaddrs.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <iomanip>
#include <mutex>

std::list<Station> StationTable::getValues(unsigned int pid) 
{
  std::list<Station> list;

  this->mutex_write.lock();
  for (auto &p : this->table)
    list.push_back(p.second);
  this->mutex_write.unlock();

  list.remove_if([&](Station &station) { return station.GetPid() <= pid; });
  return list;
}

bool StationTable::has(std::string key) 
{
  this->mutex_write.lock();
  bool found_key = this->table.find(key) != this->table.end();
  this->mutex_write.unlock();
  return found_key;
}

void StationTable::insert(std::string key, Station item)
{
  this->mutex_write.lock();
  this->has_update = true;
  this->replicate = true;
  this->clock += 1;
  this->table.insert(std::pair(key, item));
  this->table[key].update_request_retries = 0;
  this->mutex_write.unlock();
}

void StationTable::remove(std::string key)
{
  if (this->has(key))
  {
    this->mutex_write.lock();
    this->has_update = true;
    this->replicate = true;
    this->clock += 1;
    this->table.erase(key);
    this->mutex_write.unlock();
  }
}

void StationTable::update(std::string key, StationStatus new_status, StationType new_type)
{
  if (this->has(key))
  {
    this->mutex_write.lock();
    this->has_update = true;
    this->replicate = true;
    this->clock += 1;
    this->table[key].SetStatus(new_status);
    this->table[key].SetType(new_type);
    this->table[key].update_request_retries = 0;
    this->mutex_write.unlock();
  }
}

void StationTable::retry(std::string key)
{
  if (this->has(key))
  {
    this->mutex_write.lock();
    this->table[key].update_request_retries += 1;
    this->mutex_write.unlock();
  }
}



struct station_table_serial StationTable::serialize()
{
  station_table_serial serialized;
  auto stationList = this->getValues(0);

  this->mutex_write.lock();
  serialized.count = stationList.size();
  serialized.clock = this->clock;

  int i = 0;
  for (auto station : stationList)
  {
     serialized.table[i] = station.serialize();
     i++;
  }
  this->mutex_write.unlock();
  
  return serialized;
}