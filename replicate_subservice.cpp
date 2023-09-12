#include <iostream>
#include <iomanip>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <bits/stdc++.h>
#include <semaphore>
#include <iterator>
#include <list>

#include "include/replicate_subservice.h"
#include "include/datagram_subservice.h"
#include "include/sleep_server.h"
#include "include/station_table.h"

using namespace std;
using namespace datagram;

void *replicate::replicate (Station* station, MessageQueue *send_queue, StationTable *table)
{
	while(station->atomic_GetStatus() != EXITING) 
	{
        if (station->atomic_GetType() == MANAGER && table->replicate == true)
        {
            std::list<struct message> messages;

            auto table_serial = table->serialize();
            auto list = table->getValues(0);
            if (station->debug)
                std::cout << "replicate: enviando tabela para " << list.size() << " participantes" << std::endl;
            for (auto participant : list) {
                if (participant.GetMacAddress() == station->GetMacAddress())
                    continue;

                struct message replication_msg;
                replication_msg.address = participant.getAddress();
                replication_msg.sequence = 0;
                replication_msg.type = REPLICATE;
                replication_msg.table = table_serial;

                messages.push_back(replication_msg);
            }

            if (!messages.empty())
                send_queue->push(messages);
            
            table->replicate = false;
        }

        auto interval = station->atomic_GetMonitor_interval();
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
	}
	if (station->debug)
		std::cout << "saindo replicate" << std::endl;
	return 0;
}

void *replicate::load (Station* station, OperationQueue *manage_queue, MessageQueue *replicate_queue, StationTable *table)
{
    replicate_queue->mutex_read.lock();
    while(station->atomic_GetStatus() != EXITING)
    {
        replicate_queue->mutex_read.lock();
        while (!replicate_queue->queue.empty()) 
        {
            struct message msg = replicate_queue->pop();
            if (msg.type == MessageType::REPLICATE)
            {
                auto table_serial = msg.table;
                if (table_serial.clock >= table->clock)
                {
                    if (station->debug)
                        std::cout << "replicate: carregando tabela replicada" << std::endl;
                    std::list<table_operation> ops;
                    for (unsigned int i = 0; i < table_serial.count; i++)
                    {
                        auto station_serial = table_serial.table[i];
                        struct table_operation op_insert;
                        op_insert.operation = INSERT;
                        op_insert.key = station_serial.macAddress;
                        op_insert.station = Station::deserialize(station_serial);

                        ops.push_back(op_insert);
                    }
		    table->clock = table_serial.clock;
                    manage_queue->push(ops);
                }
            }
        }
    }

    return 0;
}

void replicate::multicast_replicate(Station* station, station_table_serial table_serial, 
    MessageQueue *send_queue, StationTable *table)
{
    std::list<struct message> messages;

    for (auto &s : table->getValues(0)) {
        if (s.GetMacAddress() == station->GetMacAddress())
            continue;

        struct message replication_msg;
        replication_msg.address = s.getAddress();
        replication_msg.sequence = 0;
        replication_msg.type = REPLICATE;
        replication_msg.table = table_serial;

        messages.push_back(replication_msg);
    }

    send_queue->push(messages);
}
