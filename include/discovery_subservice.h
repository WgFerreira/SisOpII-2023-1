#ifndef _DISCOVERY_H
#define _DISCOVERY_H

#include "sleep_server.h"
#include "datagram_subservice.h"
#include "management_subservice.h"

namespace discovery {
  
  /**
   * - Recebe mensagens de descoberta e responde com informações da 
   * estação líder. Adiciona e exclui linhas na tabela de hosts
   * - Envia mensagens de descoberta e espera receber informações 
   * sobre a estação líder.
   * - Implementa o algoritmo valentão
  */
  void *discovery (Station* station, datagram::DatagramQueue *datagram_queue, management::ManagementQueue *manage_queue);
};


#endif