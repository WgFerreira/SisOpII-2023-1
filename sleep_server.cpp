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

Station hostTable;
std::list<Station> list_of_stations;

std::binary_semaphore
    smphSignalManagToDiscoveryHostTable{0},
    smphSignalDiscoveryToManagHostTable{0},
    smphSignalManagToMonitoringHostTable{0},
    smphSignalMonitoringToManagHostTable{0},  
    smphSignalManagToDiscoverySetManager{0},
    smphSignalDiscoveryToManagSetManager{0}, 
    smphSignalManagToPrint{0},
    smphSignalPrintToManag{0};

int main(int argc, const char *argv[]) {
    std::cout << "Initiating Sleep Server" << std::endl;

    std::string arg = "";
    if (argv[1] != NULL)
        arg = argv[1];

    auto *station = new Station();
    station->init(arg);

    if (station->getType() == MANAGER)
    {
        auto th_management = std::async(&management::manageHostTable, station);
        auto th_discovery = std::async(&discovery::server, station);
        auto th_monitor = std::async(&monitoring::server, station);
        auto th_monitor_exit = std::async(&monitoring::exit, station);
        auto th_interface_print = std::async(&interface::print, station);
        auto th_interface_command = std::async(&interface::getCommand, station);
        th_management.wait();
        th_discovery.wait();
        th_monitor.wait();
        th_monitor_exit.wait();
        th_interface_print.wait();
        th_interface_command.wait();
    }
    else
    {
        auto th_management = std::async(&management::manageHostTable, station);
        auto th_discovery = std::async(&discovery::client, station);
        auto th_monitor = std::async(&monitoring::client, station);
        auto th_interface_print = std::async(&interface::print, station);
        auto th_interface_command = std::async(&interface::getCommand, station);
        th_management.wait();
        th_discovery.wait();
        th_monitor.wait();
        th_interface_print.wait();
        th_interface_command.wait();
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
    s.hostname = serialized.hostname;
    s.ipAddress = serialized.ipAddress;
    s.macAddress = serialized.macAddress;
    return s;
}

struct sockaddr_in Station::getSocketAddress()
{
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = inet_addr(this->ipAddress.c_str());
    memset(&(address.sin_zero), 0, 8);
    return address;
}

struct packet create_packet(PacketType type, short sequence, char* payload)
{
    struct packet packet;
    packet.type = type;
    packet.seqn = sequence;
    packet.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch() ).count();
    packet.length = strnlen(payload, 255);
    strncpy((char*) packet._payload, payload, packet.length);
    return packet;
}

int open_socket()
{
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
        std::cerr << "ERROR opening socket" << std::endl;
        
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (ret < 0)
        std::cout << "ERROR option timeout" << std::endl;
        
    int broadcastEnable=1;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
    if (ret < 0)
        std::cout << "ERROR option broadcast" << std::endl;

    return sockfd;
}

struct sockaddr_in any_address()
{
    struct sockaddr_in address;
    address.sin_family = AF_INET;     
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;
    memset(&(address.sin_zero), 0, 8);
    return address;
}

struct sockaddr_in broadcast_address()
{
    struct sockaddr_in address;
    address.sin_family = AF_INET;     
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_BROADCAST;
    memset(&(address.sin_zero), 0, 8);
    return address;
}

int recv_retry(int sockfd, void *buffer, size_t buffer_size, 
        struct sockaddr_in *addr, socklen_t *addr_len)
{
    int size = recvfrom(sockfd, buffer, buffer_size, 0, 
            (struct sockaddr *) addr, addr_len);
    if (size < 0)
        size = recvfrom(sockfd, buffer, buffer_size, 0, 
                (struct sockaddr *) addr, addr_len);
    return size;
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
    s.hostname = serialized.hostname;
    s.ipAddress = serialized.ipAddress;
    s.macAddress = serialized.macAddress;
    return s;
}

struct sockaddr_in Station::getSocketAddress()
{
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = inet_addr(this->ipAddress.c_str());
    memset(&(address.sin_zero), 0, 8);
    return address;
}

struct packet create_packet(PacketType type, short sequence, char* payload)
{
    struct packet packet;
    packet.type = type;
    packet.seqn = sequence;
    packet.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch() ).count();
    packet.length = strnlen(payload, 255);
    strncpy((char*) packet._payload, payload, packet.length);
    return packet;
}

int open_socket()
{
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
        std::cerr << "ERROR opening socket" << std::endl;
        
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (ret < 0)
        std::cout << "ERROR option timeout" << std::endl;
        
    int broadcastEnable=1;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
    if (ret < 0)
        std::cout << "ERROR option broadcast" << std::endl;

    return sockfd;
}

struct sockaddr_in any_address()
{
    struct sockaddr_in address;
    address.sin_family = AF_INET;     
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;
    memset(&(address.sin_zero), 0, 8);
    return address;
}

struct sockaddr_in broadcast_address()
{
    struct sockaddr_in address;
    address.sin_family = AF_INET;     
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_BROADCAST;
    memset(&(address.sin_zero), 0, 8);
    return address;
}

int recv_retry(int sockfd, void *buffer, size_t buffer_size, 
        struct sockaddr_in *addr, socklen_t *addr_len)
{
    int size = recvfrom(sockfd, buffer, buffer_size, 0, 
            (struct sockaddr *) addr, addr_len);
    if (size < 0)
        size = recvfrom(sockfd, buffer, buffer_size, 0, 
                (struct sockaddr *) addr, addr_len);
    return size;
}