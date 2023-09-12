#include <iostream>
#include <unistd.h>
#include <string.h>
#include <semaphore>
#include <iterator>
#include <list>
#include <cmath>

#include "include/management_subservice.h"
#include "include/sleep_server.h"

using namespace std;
using namespace management;

void *management::manage(Station* station, OperationQueue *manage_queue, StationTable *table, MessageQueue *send_queue) 
{
  manage_queue->mutex_read.lock();
  while(station->atomic_GetStatus() != EXITING) 
  {
    manage_queue->mutex_read.lock();
    while (!manage_queue->queue.empty())
    {
      if (station->debug)
        std::cout << "management: processando fila de operações na tabela" << std::endl;

      table_operation op_data = manage_queue->pop();

      switch (op_data.operation)
      {
      case INSERT:
        if (station->debug)
          std::cout << "management: inserindo nova estação" << std::endl;
        table->insert(op_data.key, op_data.station);
        break;

      case DELETE:
        if (station->debug)
          std::cout << "management: removendo uma estação se existir" << std::endl;
        table->remove(op_data.key);
        break;

      case UPDATE_STATUS:
        if (station->debug)
          std::cout << "management: atualizando status de uma estação se existir" << std::endl;
        table->update(op_data.key, op_data.new_status, op_data.new_type);
        break;
        
      case UPDATE_RETRY:
        if (station->debug)
          std::cout << "management: atualizando update retry" << std::endl;
        table->retry(op_data.key);
        break;

      default:
        break;
      }

      station->SetTable_clock(table->clock);
    }

    if (table->has_update)
    {
      table->mutex_read.unlock();
      if (station->debug)
        std::cout << "management: leitura da tabela liberada" << std::endl;
      // std::this_thread::sleep_for(std::chrono::milliseconds(100));
      // table->mutex_read.lock();
      // table->has_update = false;
    }
  }
  
  if (station->debug)
    std::cout << "saindo management" << std::endl;
  return 0;
}


