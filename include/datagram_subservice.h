#ifndef _DATAGRAM_H
#define _DATAGRAM_H

#include <list>
#include <mutex>
#include "queue.h"
#include "station.h"
#include "station_table.h"

namespace datagram {

  const int PORT = 50000;
  const int REPLICATE_PORT = 50050;

  struct packet
  {
    MessageType subservice;
    MessageType type; //Tipo do pacote (p.ex. DATA | CMD)
    unsigned short seqn; //Número de sequência
    unsigned short length; //Comprimento do payload
    unsigned long timestamp; // Timestamp do dado
    struct station_serial station;
    char _payload[255]; //Dados da mensagem
  };

  struct table_packet
  {
    MessageType subservice;
    MessageType type; //Tipo do pacote (p.ex. DATA | CMD)
    unsigned short seqn; //Número de sequência
    unsigned short length; //Comprimento do payload
    unsigned long timestamp; // Timestamp do dado
    station_table_serial station_table;
    char _payload[255]; //Dados da mensagem
  };

  /**
   * Cria um pacote de dados para ser enviado
  */
  struct packet create_packet(MessageType type, short sequence,
      struct station_serial payload);
      
  struct table_packet create_replicate_packet(MessageType type, short sequence,
      struct station_table_serial payload);

  int open_socket();
  struct sockaddr_in socket_address(in_addr_t addr);
  struct sockaddr_in socket_replicate_address(in_addr_t addr);

  /**
   * Thread que envia mensagens UDP na rede 
  */
  void *sender(Station *station, MessageQueue *send_queue);
  
  void *replicate_sender(Station *station, ReplicateMessageQueue *send_replicate_queue);

  /**
   * Thread que recebe mensagens UDP da rede 
  */
  void *receiver(Station *station, MessageQueue *discovery_queue, MessageQueue *monitor_queue);
  
  void *replicate_receiver(Station *station, ReplicateMessageQueue *replicate_queue);
}

#endif
