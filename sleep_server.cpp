#include <iostream>
#include <string>
#include <cstring>
#include <thread> // biblioteca de thread c++
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <netdb.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <semaphore>
#include <list>

// Subservices
#include "include/discovery_subservice.h"
#include "include/monitoring_subservice.h"
#include "include/management_subservice.h"
#include "include/interface_subservice.h"
#include "include/sleep_server.h"

// Constants
#define TKN_MANAGER "manager"

int main(int argc, const char *argv[]) {
    std::cout << "Initiating Sleep Server" << std::endl;

    std::string arg = "";
    if (argv[1] != NULL)
        arg = argv[1];

    auto *station = new Station();
    station->init(arg);

    auto *stationTable = new StationTable();

    struct semaphores sem;

    if (station->getType() == MANAGER)
    {
        auto th_management = std::thread(&management::manageHostTable, station, stationTable, &sem);
        auto th_discovery = std::thread(&discovery::server, station, stationTable, &sem);
        auto th_monitor = std::thread(&monitoring::server, station, stationTable, &sem);
        auto th_interface_print = std::thread(&interface::printServer, station, stationTable, &sem);
        auto th_interface_command = std::thread(&interface::getCommand, station, stationTable, &sem);
        th_management.join();
        th_discovery.join();
        th_monitor.join();
        th_interface_print.join();
        th_interface_command.join();
    }
    else
    {
        auto th_discovery = std::thread(&discovery::client, station, stationTable, &sem);
        auto th_monitor = std::thread(&monitoring::client, station);
        auto th_interface_print = std::thread(&interface::printClient, station, stationTable, &sem);
        auto th_interface_command = std::thread(&interface::getCommand, station, stationTable, &sem);
        th_discovery.join();
        th_monitor.join();
        th_interface_print.join();
        th_interface_command.join();
    }


    return 0;
}

void Station::init(std::string type)
{
    if (type == TKN_MANAGER)
        this->type = StationType::MANAGER;
    else
        this->type = StationType::PARTICIPANT;

    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    this->hostname = hostname;

    this->findInterfaceName();
    this->findMacAddress();
}

void Station::printStation()
{
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

    char macAddress[18];
	sprintf(macAddress, (const char *)"%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    this->macAddress = macAddress;
}

struct station_serial Station::serialize()
{
    struct station_serial serialized;

    serialized.status = this->status;
    strncpy(serialized.hostname, this->hostname.c_str(), HOST_NAME_MAX);
    strncpy(serialized.ipAddress, this->ipAddress.c_str(), INET_ADDRSTRLEN);
    strncpy(serialized.macAddress, this->macAddress.c_str(), MAC_ADDRESS_MAX);

    return serialized;
}

Station Station::deserialize(struct station_serial serialized)
{
    Station s;
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
    address.sin_addr.s_addr = inet_addr(this->ipAddress.c_str());
    memset(&(address.sin_zero), 0, 8);
    return address;
}
