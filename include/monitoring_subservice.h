#ifndef _MONITOR_H
#define _MONITOR_H

#include "sleep_server.h"

namespace monitoring {
  
  /**
   * Recebe mensagens de sleep status e atualiza a tabela de hosts
  */
  void *server (Station* station, StationTable* table, struct semaphores *sem);

  /**
   * Envia mensagens de sleep status
  */
  void *client (Station* station);

};

#endif
