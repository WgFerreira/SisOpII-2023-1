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
  void *discovery (Station* station, datagram::DatagramQueue *datagram_queue, management::ManagementQueue *manage_queue, StationTable *table);

  /**
   * Inicia ou termina uma eleição de líder com o algoritmo bully
  */
  void bully_algorithm(Station* station, datagram::DatagramQueue *datagram_queue, StationTable *table);
  void leader_election(Station* station, datagram::DatagramQueue *datagram_queue, StationTable *table);
  void multicast_election(Station* station, datagram::DatagramQueue *datagram_queue, StationTable *table, datagram::MessageType type, bool filter_pid);
  void election_victory(Station* station, datagram::DatagramQueue *datagram_queue, StationTable *table);

};


#endif
