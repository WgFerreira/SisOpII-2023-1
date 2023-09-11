#include <iostream>
#include <thread> // biblioteca de thread c++

// Subservices
#include "include/discovery_subservice.h"
#include "include/monitoring_subservice.h"
#include "include/management_subservice.h"
#include "include/interface_subservice.h"
#include "include/replicate_subservice.h"
#include "include/input_parser.h"
#include "include/sleep_server.h"
#include "include/station.h"

std::mutex mutex_no_manager;
std::mutex mutex_station;

auto *send_queue = new MessageQueue();
auto *discovery_queue = new MessageQueue();
auto *monitor_queue = new MessageQueue();
auto *replicate_queue = new MessageQueue();
auto *manage_queue = new OperationQueue();

auto *stationTable = new StationTable();

int main(int argc, const char *argv[]) {
    std::cout << "Initiating Sleep Server" << std::endl;

    auto *args = new InputParser();
    args->parse(argc, argv);

    auto *station = new Station();
    station->init(args);

    delete args;

    auto th_sender = std::thread(&datagram::sender, station, send_queue);
    auto th_receiver = std::thread(&datagram::receiver, station, discovery_queue, monitor_queue, replicate_queue);

    auto th_discovery = std::thread(&discovery::discovery, station, send_queue, discovery_queue, manage_queue, stationTable);
    auto th_election = std::thread(&discovery::election, station, send_queue, stationTable);

    auto th_monitor_request = std::thread(&monitoring::monitor_request, station, send_queue, manage_queue, stationTable);
    auto th_monitor_respond = std::thread(&monitoring::monitor_respond, station, send_queue, monitor_queue, manage_queue);

    // // auto th_management = std::thread(&management::manage, station, management, stationTable, datagram);

    auto th_interface = std::thread(&interface::interface, station, stationTable);
    auto th_command = std::thread(&interface::command, station, stationTable);
    
    auto th_replicate = std::thread(&replicate::replicate, station, send_queue, stationTable);
    auto th_load = std::thread(&replicate::load, station, manage_queue, replicate_queue, stationTable);

    management::manage(station, manage_queue, stationTable);

    th_sender.join();
    th_receiver.join();
    th_discovery.join();
    th_election.join();
    th_monitor_request.join();
    th_monitor_respond.join();
    // th_management.join();
    th_interface.join();
    th_command.join();
    th_replicate.join();
    th_load.join();

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

void end_all_threads_safely()
{
    mutex_no_manager.unlock();
    
    discovery_queue->mutex_read.unlock();
    monitor_queue->mutex_read.unlock();
    stationTable->has_update = true;
    manage_queue->mutex_read.unlock();
}
