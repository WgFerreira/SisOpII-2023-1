#include "include/station.h"

Station::Station()
{   // pega o time_t atual
  this->last_update = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  this->last_leader_search = 0;

  char hostname[HOST_NAME_MAX];
  gethostname(hostname, HOST_NAME_MAX);
  this->hostname = hostname;

  this->pid = getpid();

  this->findInterfaceName();
  this->findMacAddress();
}

Station::Station(struct station_serial serialized)
{   // pega o time_t atual
  this->last_update = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  this->last_leader_search = 0;
  this->deserialize(serialized);
}

void Station::init(InputParser *args)
{
  this->debug = args->debug;
  this->election_timeout = args->timeout;
  this->monitor_interval = args->monitor;
}

void Station::printStation()
{
  std::cout << "Pid: " << this->pid << std::endl;
  std::cout << "Hostname: " << this->hostname << std::endl;
  std::cout << "MAC Address: " << this->macAddress << std::endl;
  std::cout << "IP Address: " << this->ipAddress << std::endl << std::endl;
}

void Station::findInterfaceName()
{
  struct ifaddrs *addrs;
  getifaddrs(&addrs);
  for (struct ifaddrs *addr = addrs; addr != nullptr; addr = addr->ifa_next) 
  {
    if (addr->ifa_addr && addr->ifa_addr->sa_family == AF_PACKET) 
    {
      if (strncmp("en", addr->ifa_name, 2) == 0 || strncmp("eth", addr->ifa_name, 3) == 0)
      {
        this->interface = addr->ifa_name;
        break;
      }
    }
  }
  freeifaddrs(addrs);
} 

void Station::findMacAddress() 
{
	struct ifreq ifr;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy((char *)ifr.ifr_name, this->interface.c_str(), IFNAMSIZ-1);

	ioctl(fd, SIOCGIFHWADDR, &ifr);

	close(fd);
	
	unsigned char* mac = (unsigned char *)ifr.ifr_addr.sa_data;

  char macAddress[MAC_ADDRESS_MAX];
	sprintf(macAddress, (const char *)"%02x:%02x:%02x:%02x:%02x:%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  this->macAddress = macAddress;
}

struct station_serial Station::serialize()
{
  struct station_serial serialized;

  serialized.pid = this->pid;
  serialized.table_clock = this->table_clock;
  serialized.type = this->type;
  serialized.status = this->status;
  strncpy(serialized.hostname, this->hostname.c_str(), HOST_NAME_MAX);
  strncpy(serialized.ipAddress, this->ipAddress.c_str(), INET_ADDRSTRLEN);
  strncpy(serialized.macAddress, this->macAddress.c_str(), MAC_ADDRESS_MAX);

  return serialized;
}

void Station::deserialize(struct station_serial serialized)
{
  this->pid = serialized.pid;
  this->table_clock = serialized.table_clock;
  this->type = serialized.type;
  this->status = serialized.status;
  this->hostname = std::string(serialized.hostname);
  this->ipAddress = std::string(serialized.ipAddress);
  this->macAddress = std::string(serialized.macAddress);
}

struct sockaddr_in Station::getSocketAddress(int port)
{
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = this->getAddress();
  memset(&(address.sin_zero), 0, 8);
  return address;
}

in_addr_t Station::getAddress()
{
  return inet_addr(this->ipAddress.c_str());
}

auto Station::atomic_get(auto &&callback)
{
  this->mutex.lock();
  auto response = callback(this);
  this->mutex.unlock();
  return response;
}

void Station::atomic_set(auto &&callback)
{
  this->mutex.lock();
  callback(this);
  this->mutex.unlock();
}


unsigned int  Station::getPid() {
  return this->pid;
}
void          Station::SetPid(unsigned int pid) {
  atomic_set([&](Station *self) {self->pid = pid;});
}

StationType Station::GetType() {
  return atomic_get([&](Station *self) {return self->type;});
}
void Station::SetType(StationType type) {
  atomic_set([&](Station *self) {self->type = type;});
}

Station *Station::GetManager() {
  return atomic_get([&](Station *self) {return self->manager;});
}
void Station::SetManager(Station *manager) {
  atomic_set([&](Station *self) {self->manager = manager;});
}

StationStatus Station::GetStatus() {
  return atomic_get([&](Station *self) {return self->status;});
}

void          Station::SetStatus(StationStatus status) {
  atomic_set([&](Station *self) {self->status = status;});
}
