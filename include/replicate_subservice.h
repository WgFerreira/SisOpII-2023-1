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
  void *replicate (Station* station, MessageQueue *send_queue, StationTable *table);
  void *load (Station* station, OperationQueue *manage_queue, MessageQueue *replicate_queue, StationTable *table);

  void multicast_replicate(Station* station, station_table_serial table_serial, MessageQueue *send_queue, StationTable *table);
};

#endif
