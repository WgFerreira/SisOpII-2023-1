#ifndef _DATAGRAM_H
#define _DATAGRAM_H

#include <list>
#include <mutex>
#include "sleep_server.h"

namespace datagram {

  const int PORT = 50000;

  enum MessageType: unsigned short
  {
    // Identifica qual subserviço recebe a mensagem
    DISCOVERY, 
    MONITORING,
    // Identifica o que a mensagem é
    MANAGER_ELECTION,
    ELECTION_ANSWER,
    ELECTION_VICTORY,
    STATUS_REQUEST,
    STATUS_RESPONSE,
    LEAVING
  };

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
    // monitoring::station_table_serial station_table;
    char _payload[255]; //Dados da mensagem
  };

  struct message
  {
    in_addr_t address;
    MessageType type;
    Station payload;
    short sequence;
  };

  class DatagramQueue
  {
  public:
    std::mutex mutex_sending;
    std::list<struct message> sending_queue;

    std::mutex mutex_discovery;
    std::list<struct message> discovery_queue;

    std::mutex mutex_monitoring;
    std::list<struct message> monitoring_queue;

    std::mutex mutex_sync;
    std::list<struct message> sync_queue;
  };
  
  /**
   * Cria um pacote de dados para ser enviado
  */
  struct packet create_packet(MessageType type, short sequence,
      struct station_serial payload);

  int open_socket();
  struct sockaddr_in socket_address(in_addr_t addr);

  std::string messageTypeToString(MessageType type);

  /**
   * Thread que envia mensagens UDP na rede 
  */
  void *sender(Station *station, DatagramQueue *datagram_queue);

  /**
   * Thread que recebe mensagens UDP da rede 
  */
  void *receiver(Station *station, DatagramQueue *datagram_queue);
}

#endif
