#include "include/station.h"

#include <iostream>
#include <cstring>
#include <ifaddrs.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <iomanip>

using namespace std;

void Station::init(InputParser *args)
{
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    this->hostname = hostname;

    this->pid = getpid();

    this->findInterfaceName();
    this->findMacAddress();

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

void Station::print_interface()
{
    std::string status = "";
    switch (this->status)
    {
    case AWAKEN:
        status = "AWAKEN";
        break;
    case ASLEEP:
        status = "SLEEPING";
        break;
    case ELECTING:
        status = "ELECTING";
        break;
    case WAITING_ELECTION:
        status = "WAITING ELECTION";
        break;
    default:
        status = "UNKNOWN";
    }

    std::string type = "";
    switch (this->type)
    {
    case MANAGER:
        type = " *";
        break;
    case CANDIDATE:
        type = " ?";
        break;
    case PARTICIPANT:
        type = " ";
        break;
    
    default:
        status = "##";
    }
    
	const char separator = ' ';
    std::cout << std::left << std::setw(3) << std::setfill(separator) << type;
    std::cout << std::left << std::setw(30) << std::setfill(separator) << this->hostname;
    std::cout << std::left << std::setw(20) << std::setfill(separator) << this->macAddress;
    std::cout << std::left << std::setw(20) << std::setfill(separator) << this->ipAddress;
    std::cout << std::left << std::setw(10) << std::setfill(separator) << status;
    std::cout << std::left << std::setw(10) << std::setfill(separator) << this->pid;
    std::cout << std::endl;
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

Station Station::deserialize(struct station_serial serialized)
{
    Station s;
    s.pid = serialized.pid;
    s.table_clock = serialized.table_clock;
    s.type = serialized.type;
    s.status = serialized.status;
    s.hostname = std::string(serialized.hostname);
    s.ipAddress = std::string(serialized.ipAddress);
    s.macAddress = std::string(serialized.macAddress);
    return s;
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
  mutex_station.lock();
  auto response = callback(this);
  mutex_station.unlock();
  return response;
}

void Station::atomic_set(std::function<void(Station *)> callback)
{
  mutex_station.lock();
  callback(this);
  mutex_station.unlock();
}

StationType Station::atomic_GetType() {
  return atomic_get([&](Station *self) {return self->type;});
}
void Station::atomic_SetType(StationType type) {
  atomic_set([&](Station *self) {self->type = type;});
}

Station *Station::atomic_GetManager() {
  return atomic_get([&](Station *self) {return self->manager;});
}
void Station::atomic_SetManager(Station *manager) {
  atomic_set([&](Station *self) {self->manager = manager;});
}

StationStatus Station::atomic_GetStatus() {
  return atomic_get([&](Station *self) {return self->status;});
}

void          Station::atomic_SetStatus(StationStatus status) {
  atomic_set([&](Station *self) {self->status = status;});
}

int   Station::atomic_GetLeader_search_retries() {
  return atomic_get([&](Station *self) {return self->leader_search_retries;});
}

void  Station::atomic_SetLeader_search_retries(int leader_search_retries) {
  atomic_set([&](Station *self) {self->leader_search_retries = leader_search_retries;});
}

u_int64_t Station::atomic_GetLast_leader_search() {
  return atomic_get([&](Station *self) {return self->last_leader_search;});
}

void      Station::atomic_SetLast_leader_search(u_int64_t last_leader_search) {
  atomic_set([&](Station *self) {self->last_leader_search = last_leader_search;});
}

u_int64_t Station::atomic_GetElection_timeout() {
  return atomic_get([&](Station *self) {return self->election_timeout;});
}

void      Station::atomic_SetElection_timeout(u_int64_t election_timeout) {
  atomic_set([&](Station *self) {self->election_timeout = election_timeout;});
}

u_int64_t Station::atomic_GetLast_update() {
  return atomic_get([&](Station *self) {return self->last_update;});
}

void      Station::atomic_SetLast_update(u_int64_t last_update) {
  atomic_set([&](Station *self) {self->last_update = last_update;});
}

u_int64_t Station::atomic_GetMonitor_interval() {
  return atomic_get([&](Station *self) {return self->monitor_interval;});
}

void      Station::atomic_SetMonitor_interval(u_int64_t monitor_interval) {
  atomic_set([&](Station *self) {self->monitor_interval = monitor_interval;});
}