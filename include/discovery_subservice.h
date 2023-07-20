#ifndef _DISCOVERY_H
#define _DISCOVERY_H

#include "sleep_server.h"

namespace discovery {
  
  /**
   * Recebe mensagens de descoberta e responde com informações da 
   * estação líder. Adiciona e exclui linhas na tabela de hosts
  */
  void *server (Station* station, StationTable* table, struct semaphores *sem);

  /**
   * Envia mensagens de descoberta e espera receber informações 
   * sobre a estação líder
  */
  void *client (Station* station);

};


#endif