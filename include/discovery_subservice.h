#ifndef _DISCOVERY_H
#define _DISCOVERY_H

#include "station.h"
#include "queue.h"
#include "management_subservice.h"
// #include "sleep_server.h"
// #include "datagram_subservice.h"

namespace discovery {

  /**
   * - Recebe mensagens de descoberta e responde com informações da 
   * estação líder. Adiciona e exclui linhas na tabela de hosts
   * - Envia mensagens de descoberta e espera receber informações 
   * sobre a estação líder.
   * - Implementa o algoritmo valentão
  */
  void *discovery (Station* station, MessageQueue *send_queue, MessageQueue *discovery_queue, OperationQueue *manage_queue, management::StationTable *table);


  /**
   * Inicia ou termina uma eleição de líder com o algoritmo bully
  */
  void *election (Station* station, MessageQueue *send_queue, management::StationTable *table);
  void leader_election(Station* station, MessageQueue *send_queue, management::StationTable *table);
  void multicast_election(Station* station, MessageQueue *send_queue, management::StationTable *table, MessageType type, bool filter_pid);
  void election_victory(Station* station, MessageQueue *send_queue, management::StationTable *table);

};


#endif