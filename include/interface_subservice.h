#ifndef _INTERFACE_H
#define _INTERFACE_H

#include <unistd.h>
#include <string.h>

#include "sleep_server.h"

namespace interface {
  
  /**
   * Apresenta tabela de hosts apara a estação líder, ou informações da
   * estação líder para a estação participante
  */
  void *print (Station* station, StationTable* table, struct semaphores *sem);

  /**
   * Espera comando EXIT ou WAKEUP host
  */
  void *getCommand (Station* station, StationTable* table, struct semaphores *sem);

};

#endif
