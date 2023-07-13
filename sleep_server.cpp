#include <iostream>
#include <string>
#include <cstring>
#include <future> // biblioteca de thread c++
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <netdb.h>
#include <unistd.h>

// Subservices
#include "include/discovery_subservice.h"
#include "include/monitoring_subservice.h"
#include "include/management_subservice.h"
#include "include/interface_subservice.h"
#include "include/sleep_server.h"

// Constants
#define TKN_MANAGER "manager"

int main(int argc, const char *argv[]) {

    auto *station = new Station();
    if (argv[1] != NULL)
        station->init(argv[1]);    

    if (station->type == MANAGER)
    {
        auto th_discovery = std::async(&discovery::server, station);
        th_discovery.wait();
    }
    else
    {
        auto th_discovery = std::async(&discovery::client, station);
        th_discovery.wait();
    }


    return 0;
}

void Station::init(std::string arg)
{
    if (arg == TKN_MANAGER)
    {
        this->type = StationType::MANAGER;
    }
    else 
    {
        this->type = StationType::PARTICIPANT;
    }

    this->macAddress = this->extractMacAddress();
}

std::string Station::extractMacAddress() 
{
	struct ifreq ifr;
	
	int fd = socket(AF_INET, SOCK_DGRAM, 0);

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy((char *)ifr.ifr_name, "eth0", IFNAMSIZ-1);

	ioctl(fd, SIOCGIFHWADDR, &ifr);

	close(fd);
	
	unsigned char* mac = (unsigned char *)ifr.ifr_addr.sa_data;
	
    char buffer[18];
	sprintf(buffer, (const char *)"%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buffer;
}
