#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "sleep_server.h"

namespace interface {
  
  /**
   * Apresenta tabela de hosts apara a estação líder, ou informações da
   * estação líder para a estação participante
  */
  void *print (Station* station);

  /**
   * Espera comando EXIT ou WAKEUP host
  */
  void *getCommand (Station* station);

};

#endif