#ifndef _MANAGEMENT_H
#define _MANAGEMENT_H

#include "datagram_subservice.h"
#include <map>
#include <list>

#include "sleep_server.h"
#include "station_table.h"

namespace management {
  /**
   * Realiza as operações na tabela de estações
   * 
  */
  
  void *manage(Station* station, OperationQueue *manage_queue, StationTable *table, MessageQueue *send_queue, ReplicateMessageQueue *replicate_queue);

};

#endif
