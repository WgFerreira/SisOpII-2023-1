#include <iostream>
#include <unistd.h>
#include <string.h>
#include <semaphore>
#include <iterator>
#include <list>

#include "include/management_subservice.h"
#include "include/sleep_server.h"

using namespace std;

void *management::manageHostTable (Station* station, StationTable* table, struct semaphores *sem) 
{
    while(station->status != EXITING) 
    {
        if (table->has_update)
        {
            sem->mutex_read.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            sem->mutex_read.lock();
            table->has_update = false;
        }

        if (table->buffer.operation != NONE)
        {
            if (sem->mutex_write.try_lock())
            {

                table->has_update = true;
                if (table->buffer.operation == INSERT)
                {
                    table->table.insert(std::pair<std::string,Station>(table->buffer.key, table->buffer.station));
                    table->table[table->buffer.key].last_update = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch() ).count();
                }
                else 
                if (table->buffer.operation == DELETE)
                    table->table.erase(table->buffer.key);
                else 
                if (table->buffer.operation == UPDATE_STATUS)
                {
                    table->table[table->buffer.key].status = table->buffer.new_status;
                    table->table[table->buffer.key].last_update = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch() ).count();
                }
                table->buffer.operation = NONE;

                sem->mutex_buffer.unlock();
            }
        }
    }
    
    if (station->debug)
        std::cout << "saindo management" << std::endl;
}

