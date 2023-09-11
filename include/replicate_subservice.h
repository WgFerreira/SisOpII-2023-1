#ifndef _REPLICATE_H
#define _REPLICATE_H

#include <unistd.h>
#include <string.h>

#include "datagram_subservice.h"
#include "sleep_server.h"
#include "station_table.h"

namespace replicate {
  
  /**
   * Replica a tabela de participantes para todos os participantes
  */
  void *replicate (Station* station, OperationQueue *manage_queue, ReplicateMessageQueue *replicate_queue, StationTable *table);

};

#endif
