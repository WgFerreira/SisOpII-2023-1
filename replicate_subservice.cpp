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

void *replicate::replicate (Station* station, OperationQueue *manage_queue, ReplicateMessageQueue *replicate_queue, StationTable *table)
{
	while(station->status != EXITING) 
	{
		if (replicate_queue->mutex_read.try_lock())
		{
			if (!station->debug)
			{
                               if (station->getType() == MANAGER) {
                                    for (auto &tupla : table->table)
                                    {
                                        Station s = tupla.second;
                                        if (s.hostname.length() > 0)
                                        {
                                            struct replicate_message replicate_msg;
                                            replicate_msg.address = s.getAddress();
                                            replicate_msg.sequence = 0;
                                            replicate_msg.type = REPLICATE;
                                            replicate_msg.payload = table;

                                            replicate_queue->push(replicate_msg);
                                        }
                                    }
                                } else {
                                    if (!replicate_queue->queue.empty()) {
                                        struct replicate_message msg = replicate_queue->pop();
                                        
                                        for (auto &tupla : msg.payload->table)
                                        {
                                            Station s = tupla.second;
                                            if (s.hostname.length() > 0)
                                            {
                                                struct table_operation op_delete;
                                                op_delete.operation = DELETE;
                                                op_delete.key = s.macAddress;
                                                
                                                manage_queue->push(op_delete);
                                                
                                                struct table_operation op_insert;
                                                op_insert.operation = INSERT;
                                                op_insert.key = s.macAddress;
                                                op_insert.station = s;

                                                manage_queue->push(op_insert);
                                            }
                                        }
                                    }
                                }
			}
			else 
			{
				// station->printStation();
				cout << "replicate: table has update " << table->has_update << endl;
			}
		}
	}
	if (station->debug)
		std::cout << "saindo replicate" << std::endl;
	return 0;
}
