#ifndef _REPLICATE_H
#define _REPLICATE_H

#include <unistd.h>
#include <string.h>

#include "management_subservice.h"
#include "datagram_subservice.h"
#include "sleep_server.h"

namespace replicate {
  
  /**
   * Replica a tabela de participantes para todos os participantes
  */
  void *replicate (Station* station, datagram::DatagramQueue *datagram_queue, StationTable *table);

};

#endif
