#include <iostream>
#include <string>
#include <future> // biblioteca de thread c++

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
    station->init(argv[1]);    

    auto th_discovery = std::async(&discovery, station);

    th_discovery.wait();


    return 0;
}

void Station::init(std::string arg)
{
    if (arg == "manager")
        this->type = StationType::MANAGER;
    else
        this->type = StationType::PARTICIPANT;
}
