#ifndef _DATAGRAM_H
#define _DATAGRAM_H

#include <list>
#include <mutex>
#include "queue.h"
#include "station.h"
#include "station_table.h"

namespace datagram {

  const int PORT = 50000;

  union station_or_table {
    station_serial s;
    station_table_serial t;
  };

  struct packet
  {
    MessageType subservice;
    MessageType type; //Tipo do pacote (p.ex. DATA | CMD)
    unsigned short seqn; //Número de sequência
    unsigned short length; //Comprimento do payload
    unsigned long timestamp; // Timestamp do dado
    station_or_table payload;
    char _payload[255]; //Dados da mensagem
  };

  /**
   * Cria um pacote de dados para ser enviado
  */
  struct packet create_packet(MessageType type, short sequence,
      station_or_table payload);

  int open_socket();
  struct sockaddr_in socket_address(in_addr_t addr);

  /**
   * Thread que envia mensagens UDP na rede 
  */
  void *sender(Station *station, MessageQueue *send_queue);

  /**
   * Thread que recebe mensagens UDP da rede 
  */
  void *receiver(Station *station, MessageQueue *discovery_queue, MessageQueue *monitor_queue, MessageQueue *replicate_queue);
}

#endif
