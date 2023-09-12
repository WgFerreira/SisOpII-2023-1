#ifndef _MONITOR_H
#define _MONITOR_H

#include "sleep_server.h"
#include "station_table.h"
#include "datagram_subservice.h"
#include "management_subservice.h"

namespace monitoring {
  
  /**
   * Recebe mensagens de sleep status e atualiza a tabela de hosts a cada x ms
   * O manager atualiza o status de cada participante
   * O participante monitora se o manager ainda está perguntando
  */
  void *monitor_request (Station* station, MessageQueue *send_queue, OperationQueue *manage_queue, StationTable *table);
  void *monitor_respond (Station* station, MessageQueue *send_queue, MessageQueue *monitor_queue, OperationQueue *manage_queue);

};

#endif
