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

using namespace std;
using namespace datagram;

void *replicate::replicate (Station* station, datagram::DatagramQueue *datagram_queue, StationTable *table)
{
	while(station->status != EXITING) 
	{
		if (datagram_queue->mutex_replicate_read.try_lock())
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

                            datagram_queue->mutex_replicate_sending.lock();
                            datagram_queue->sending_replicate_queue.push_back(replicate_msg);
                            datagram_queue->mutex_replicate_sending.unlock();
                            /*
                            cout << left << setw(nameWidth) << setfill(separator) << s.hostname;
                            cout << left << setw(nameWidth) << setfill(separator) << s.macAddress;
                            cout << left << setw(nameWidth) << setfill(separator) << s.ipAddress;
                            cout << left << setw(nameWidth) << setfill(separator) << status;
                            cout << endl;
                            */
                        }
                    }
                } else {
                    if (!datagram_queue->replicate_queue.empty()) {
                        datagram_queue->mutex_replicate.lock();
                        struct replicate_message msg = datagram_queue->replicate_queue.front();
                        datagram_queue->replicate_queue.pop_front();
                        datagram_queue->mutex_replicate.unlock();

                        

                    }
                }
			}
			else 
			{
				// station->printStation();
				cout << "interface: table has update " << table->has_update << endl;
			}
			datagram_queue->mutex_replicate_read.unlock();
		}
	}
	if (station->debug)
		std::cout << "saindo replicate" << std::endl;
	return 0;
}
