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
        if (station->atomic_GetType() == MANAGER)
        {
            std::list<struct message> messages;

            auto table_serial = table->serialize();
            auto list = table->getValues(0);
            for (auto participant : list) {
                if (participant.GetMacAddress() == station->GetMacAddress())
                    continue;

                struct message replication_msg;
                replication_msg.address = participant.getAddress();
                replication_msg.sequence = 0;
                replication_msg.type = REPLICATE;
                replication_msg.payload = table_serial;
                
                messages.push_back(replication_msg);
            }

            if (!messages.empty())
                send_queue->push(messages);
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
                auto table_serial = std::get<station_table_serial>(msg.payload);
                if (table_serial.clock > table->clock)
                {
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
                    manage_queue->push(ops);
                }
            }
        }
    }

    return 0;
}



		// if (replicate_queue->mutex_read.try_lock())
		// {
		// 	if (!station->debug)
		// 	{
        //                        if (station->getType() == MANAGER) {
        //                             for (auto &tupla : table->table)
        //                             {
        //                                 Station s = tupla.second;
        //                                 if (s.hostname.length() > 0)
        //                                 {
        //                                     struct replicate_message replicate_msg;
        //                                     replicate_msg.address = s.getAddress();
        //                                     replicate_msg.sequence = 0;
        //                                     replicate_msg.type = REPLICATE;
        //                                     replicate_msg.payload = table;

        //                                     replicate_queue->push(replicate_msg);
        //                                 }
        //                             }
        //                         } else {
        //                             if (!replicate_queue->queue.empty()) {
        //                                 struct replicate_message msg = replicate_queue->pop();
                                        
        //                                 for (auto &tupla : msg.payload->table)
        //                                 {
        //                                     Station s = tupla.second;
        //                                     if (s.hostname.length() > 0)
        //                                     {
        //                                         struct table_operation op_delete;
        //                                         op_delete.operation = DELETE;
        //                                         op_delete.key = s.macAddress;
                                                
        //                                         manage_queue->push(op_delete);
                                                
        //                                         struct table_operation op_insert;
        //                                         op_insert.operation = INSERT;
        //                                         op_insert.key = s.macAddress;
        //                                         op_insert.station = s;

        //                                         manage_queue->push(op_insert);
        //                                     }
        //                                 }
        //                             }
        //                         }
		// 	}
		// 	else 
		// 	{
		// 		// station->printStation();
		// 		cout << "replicate: table has update " << table->has_update << endl;
		// 	}
		// }

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
        replication_msg.payload = table_serial;
        
        messages.push_back(replication_msg);
    }

    send_queue->push(messages);
}

