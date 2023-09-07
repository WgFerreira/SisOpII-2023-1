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
// #include "include/monitoring_subservice.h"
#include "include/management_subservice.h"
#include "include/interface_subservice.h"
#include "include/input_parser.h"
#include "include/sleep_server.h"

int main(int argc, const char *argv[]) {
    std::cout << "Initiating Sleep Server" << std::endl;

    auto *args = new InputParser();
    args->parse(argc, argv);

    auto *station = new Station();
    station->init(args);

    delete args;

    auto *stationTable = new management::StationTable();

    auto *datagram = new datagram::DatagramQueue();
    auto *management = new management::ManagementQueue();

    auto th_sender = std::thread(&datagram::sender, station, datagram);
    auto th_receiver = std::thread(&datagram::receiver, station, datagram);
    auto th_discovery = std::thread(&discovery::discovery, station, datagram, management, stationTable);
    auto th_management = std::thread(&management::manage, station, management, stationTable, datagram);
    auto th_interface = std::thread(&interface::interface, station, stationTable);
    auto th_command = std::thread(&interface::command, station, stationTable);

    th_sender.join();
    th_receiver.join();
    th_discovery.join();
    th_management.join();
    th_interface.join();
    th_command.join();

    return 0;
}

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

uint64_t now()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

uint64_t millis_since(u_int64_t then)
{
    return now() - then;
}
