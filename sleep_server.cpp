#include <iostream>
#include <thread> // biblioteca de thread c++

#include "include/sleep_server.h"
#include "include/station.h"
#include "include/input_parser.h"

// Subservices
// #include "include/discovery_subservice.h"
// #include "include/monitoring_subservice.h"
// #include "include/management_subservice.h"
// #include "include/interface_subservice.h"

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

    auto th_discovery = std::thread(&discovery::discovery, station, management, stationTable);
    auto th_monitor = std::thread(&monitoring::monitor, station, management, stationTable);
    auto th_interface = std::thread(&interface::interface, station, stationTable);

    management::manage(station, management, stationTable);

    th_discovery.join();
    th_monitor.join();
    th_interface.join();

    return 0;
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
