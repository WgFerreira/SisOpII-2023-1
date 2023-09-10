#ifndef _MONITOR_H
#define _MONITOR_H

#include "sleep_server.h"
#include "datagram_subservice.h"
#include "management_subservice.h"

namespace monitoring {
  
  /**
   * Recebe mensagens de sleep status e atualiza a tabela de hosts a cada x ms
   * O manager atualiza o status de cada participante
   * O participante monitora se o manager ainda está perguntando
  */
  void *monitor (Station* station, datagram::DatagramQueue *datagram_queue, management::ManagementQueue *queue, management::StationTable *table);

};

#endif
