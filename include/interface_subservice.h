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
  void *print (Station* station);

  /**
   * Espera comando EXIT ou WAKEUP host
  */
  void *getCommand (Station* station);
  
  /**
    * Envia pacote mágico WakeOnLan para um computador
  */
  void *wakeonlan (std::string mac_address);
  
  /**
    * Informa ao MANAGER que está saindo do serviço e encerra a execeução do mesmo
  */
  void *exit_service(Station* station);

};

#endif
