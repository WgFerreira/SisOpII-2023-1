#include "include/datagram_subservice.h"

#include <iostream>
#include <cstring>

using namespace datagram;

void *datagram::sender(Station *station, MessageQueue *send_queue)
{
  int sockfd = datagram::open_socket();
  bool running = true;

  send_queue->mutex_read.lock();
  while (running)
  {
    send_queue->mutex_read.lock();
    while (!send_queue->queue.empty())
    {
      struct message msg = send_queue->pop();

      struct sockaddr_in sock_addr = socket_address(msg.address);
      
      station_or_table payload;
      if (msg.type == REPLICATE)
        payload.t = std::get<station_table_serial>(msg.payload);
      else {
        Station s = std::get<Station>(msg.payload);
        payload.s = s.serialize();
      }
      struct packet data = create_packet(msg.type, 0, payload);

      if (station->debug)
      {
        char ipAddress[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sock_addr.sin_addr), ipAddress, INET_ADDRSTRLEN);
        std::cout << "sending a message " << messageTypeToString(data.type)
            << " to " << ipAddress << std::endl;
      } 

      int n = sendto(sockfd, &data, sizeof(data), 0, (const struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in));
      if (n < 0) 
        std::cout << "ERROR sending message" << std::endl;

      if (msg.type == LEAVING)
      {
        running = false;
        break;
      }
    }
  }
  
  close(sockfd);
  if (station->debug)
    std::cout << "sender: saindo" << std::endl;
  return 0;
}

void *datagram::receiver(Station *station, MessageQueue *discovery_queue, MessageQueue *monitor_queue, MessageQueue *replicate_queue)
{
  int sockfd = datagram::open_socket();
  
  struct sockaddr_in bound_addr = socket_address(INADDR_ANY);
  if (bind(sockfd, (struct sockaddr *) &bound_addr, sizeof(struct sockaddr)) < 0) 
      std::cerr << "ERROR binding socket" << std::endl;

  while (station->atomic_GetStatus() != EXITING)
  {
    struct sockaddr_in client_addr;
    struct packet client_data;
    socklen_t client_addr_len = sizeof(struct sockaddr_in);

    int n = recvfrom(sockfd, &client_data, sizeof(struct packet), 0, (struct sockaddr *) &client_addr, &client_addr_len);
    if (n > 0)
    {
      struct message msg;
      msg.type = client_data.type;
      msg.address = client_addr.sin_addr.s_addr;
      msg.sequence = client_data.seqn;

      if (client_data.subservice == REPLICATION) {
        msg.payload = client_data.payload.t;
      }
      else {
        station_serial s = client_data.payload.s;

        if (s.pid == station->GetPid())
          continue;

        inet_ntop(AF_INET, &(client_addr.sin_addr), s.ipAddress, INET_ADDRSTRLEN);
        Station participant = Station::deserialize(s);

        if (station->debug)
          std::cout << "a message was received " << messageTypeToString(client_data.type) 
              << " from " << s.ipAddress << std::endl;

        msg.payload = participant;
      }

      switch (client_data.subservice)
      {
        case DISCOVERY:
          discovery_queue->push(msg);
          break;

        case REPLICATION:
          replicate_queue->push(msg);
          break;

        case MONITORING:
        default:
          monitor_queue->push(msg);
      }
    }
  }

  close(sockfd);
  if (station->debug)
    std::cout << "receiver: saindo" << std::endl;
  return 0;
}

struct packet datagram::create_packet(MessageType type, short sequence, 
    station_or_table payload)
{
  struct packet packet;
  if (type == STATUS_REQUEST || type == STATUS_RESPONSE)
    packet.subservice = MONITORING;
  else
    packet.subservice = DISCOVERY;
  packet.type = type;
  packet.seqn = sequence;
  packet.timestamp = now();
  packet.length = sizeof(payload);
  packet.payload = payload;
  return packet;
}

int datagram::open_socket()
{
  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    std::cerr << "ERROR opening socket" << std::endl;
      
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0; // 500ms
  int ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  if (ret < 0)
    std::cout << "ERROR option timeout" << std::endl;
      
  int broadcastEnable = 1;
  ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
  if (ret < 0)
    std::cout << "ERROR option broadcast" << std::endl;

  return sockfd;
}

struct sockaddr_in datagram::socket_address(in_addr_t addr)
{
  struct sockaddr_in address;
  address.sin_family = AF_INET;     
  address.sin_port = htons(PORT);
  address.sin_addr.s_addr = addr;
  memset(&(address.sin_zero), 0, 8);
  return address;
}
