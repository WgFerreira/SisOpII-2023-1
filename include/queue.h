#ifndef _QUEUE_H
#define _QUEUE_H

#include <mutex>
#include <list>
#include <variant>
#include <arpa/inet.h>
#include <ifaddrs.h>

#include "station.h"
#include "station_table.h"
#include "sleep_server.h"

using namespace std;

enum MessageType: unsigned short
{
  // Identifica qual subserviço recebe a mensagem
  DISCOVERY, 
  MONITORING,
  REPLICATION,
  // Identifica o que a mensagem é
  MANAGER_ELECTION,
  ELECTION_ANSWER,
  ELECTION_VICTORY,
  STATUS_REQUEST,
  STATUS_RESPONSE,
  LEAVING,
  REPLICATE
};

union station_or_table {
  Station s;
  StationTable t;
};

struct message
{
  in_addr_t address;
  MessageType type;
  std::variant<Station, station_table_serial> payload;
  short sequence;
};

class MessageQueue
{
public:
  mutex mutex_write;
  mutex mutex_read;
  list<message> queue;

  void push(message m);
  void push(list<message> m);
  message pop();
};

std::string messageTypeToString(MessageType type);

// --------------

enum TableOperation: uint16_t
{
  INSERT,         /** key, station */
  UPDATE_STATUS,  /** key, new_status, new_type */
  UPDATE_RETRY,  /** key, update_retries */
  DELETE,         /** key */
  NONE
};

struct table_operation
{
  TableOperation operation;
  std::string key;
  Station station;
  StationStatus new_status;
  StationType new_type;
};

class OperationQueue
{
public:
  mutex mutex_write;
  mutex mutex_read;
  list<table_operation> queue;

  void push(table_operation m);
  void push(list<table_operation> m);
  table_operation pop();
};

#endif
