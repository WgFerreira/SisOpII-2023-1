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
    bool has_update = false;
    while(station->status != EXITING) 
    {	
        if (has_update)
        {
            sem->mutex_read.unlock();
            sem->mutex_read.lock();
            has_update = false;
        }

        if (station->getType() == MANAGER)
        {
            sem->mutex_write.lock();

            if (table->buffer.operation != NONE)
            {
                has_update = true;
                if (table->buffer.operation == INSERT)
                    table->table.insert(std::pair<std::string,Station>(table->buffer.key, table->buffer.station));
                else 
                if (table->buffer.operation == DELETE)
                    table->table.erase(table->buffer.key);
                else 
                if (table->buffer.operation == UPDATE_STATUS)
                    table->table[table->buffer.key].status = table->buffer.new_status;
                table->buffer.operation = NONE;
            }

            sem->mutex_buffer.unlock();
        }
        else 
        {
            sem->mutex_write.lock();
            has_update = true;
            sem->mutex_buffer.unlock();
        }
    }
    
}

