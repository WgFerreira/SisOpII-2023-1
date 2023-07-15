#include <iostream>
#include <string>
#include <cstring>
#include <future> // biblioteca de thread c++
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <netdb.h>
#include <unistd.h>
#include <ifaddrs.h>

// Subservices
#include "include/discovery_subservice.h"
#include "include/monitoring_subservice.h"
#include "include/management_subservice.h"
#include "include/interface_subservice.h"
#include "include/sleep_server.h"

// Constants
#define TKN_MANAGER "manager"

struct station_entry hostTable;

int main(int argc, const char *argv[]) {
    std::cout << "Initiating Sleep Server" << std::endl;

    auto *station = new Station();
    if (argv[1] != NULL)
        station->init(argv[1]);

    gethostname(station->hostname, HOST_NAME_MAX);
    if (station->type == StationType::MANAGER)
        std::cout << "MANAGER Station " << station->hostname << std::endl;
    else
        std::cout << "PARTICIPANT Station " << station->hostname << std::endl;

    station->extractMacAddress();

    if (station->type == MANAGER)
    {
        auto th_discovery = std::async(&discovery::server, station);
        auto th_monitor = std::async(&monitoring::server, station);
        th_discovery.wait();
        th_monitor.wait();
    }
    else
    {
        auto th_discovery = std::async(&discovery::client, station);
        auto th_monitor = std::async(&monitoring::client, station);
        th_discovery.wait();
        th_monitor.wait();
    }


    return 0;
}

void Station::init(std::string arg)
{
    if (arg == TKN_MANAGER)
        this->type = StationType::MANAGER;
    else
        this->type = StationType::PARTICIPANT;
}

void Station::extractMacAddress() 
{
    std::string interface_name = "eth0";

    struct ifaddrs *addrs;
    getifaddrs(&addrs);
    for (struct ifaddrs *addr = addrs; addr != nullptr; addr = addr->ifa_next) 
    {
        if (addr->ifa_addr && addr->ifa_addr->sa_family == AF_PACKET) 
        {
            if (strncmp("en", addr->ifa_name, 2) == 0 || strncmp("eth", addr->ifa_name, 3) == 0)
            {
                interface_name = addr->ifa_name;
                break;
            }
        }
    }
    freeifaddrs(addrs);

	struct ifreq ifr;
	
	int fd = socket(AF_INET, SOCK_DGRAM, 0);

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy((char *)ifr.ifr_name, interface_name.c_str(), IFNAMSIZ-1);

	ioctl(fd, SIOCGIFHWADDR, &ifr);

	close(fd);
	
	unsigned char* mac = (unsigned char *)ifr.ifr_addr.sa_data;
	
	sprintf(this->macAddress, (const char *)"%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    std::cout << "Interface Name: " << interface_name << "\tMAC Address: " << this->macAddress << std::endl;
}
