#include "include/datagram_subservice.h"

#include <iostream>
#include <cstring>

using namespace datagram;

void *datagram::sender(Station *station, DatagramQueue *datagram_queue)
{
  int sockfd = datagram::open_socket();

  while (station->status != EXITING || !datagram_queue->sending_queue.empty())
  {
    if (!datagram_queue->sending_queue.empty() && datagram_queue->mutex_sending.try_lock())
    {
      struct message msg = datagram_queue->sending_queue.front();
      datagram_queue->sending_queue.pop_front();
      datagram_queue->mutex_sending.unlock();

      struct sockaddr_in sock_addr = socket_address(msg.address);
      struct packet data = create_packet(msg.type, 0, msg.payload.serialize());

      if (station->debug)
        std::cout << "sending a message" << std::endl;

      int n = sendto(sockfd, &data, sizeof(data), 0, (const struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in));
      if (n < 0) 
        std::cout << "ERROR sending message" << std::endl;
    }

    if (!datagram_queue->sending_replicate_queue.empty() && datagram_queue->mutex_replicate_sending.try_lock())
    {
      struct replicate_message msg = datagram_queue->sending_replicate_queue.front();
      datagram_queue->sending_replicate_queue.pop_front();
      datagram_queue->mutex_replicate_sending.unlock();

      struct sockaddr_in sock_addr = socket_address(msg.address);
      struct table_packet data = create_replicate_packet(msg.type, 0, msg.payload->serialize());

      if (station->debug)
        std::cout << "sending a message" << std::endl;

      int n = sendto(sockfd, &data, sizeof(data), 0, (const struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in));
      if (n < 0) 
        std::cout << "ERROR sending message" << std::endl;
    }
  }
  
  close(sockfd);
  return 0;
}

void *datagram::receiver(Station *station, DatagramQueue *datagram_queue)
{
  int sockfd = datagram::open_socket();
  
  struct sockaddr_in bound_addr = socket_address(INADDR_ANY);
  if (bind(sockfd, (struct sockaddr *) &bound_addr, sizeof(struct sockaddr)) < 0) 
      std::cerr << "ERROR binding socket" << std::endl;

  while (station->status != EXITING)
  {
    struct sockaddr_in client_addr;
    struct packet client_data;
    struct table_packet client_replicate_data;
    socklen_t client_addr_len = sizeof(struct sockaddr_in);

    int n = recvfrom(sockfd, &client_data, sizeof(struct packet), 0, (struct sockaddr *) &client_addr, &client_addr_len);
    if (n > 0)
    {
      if (client_data.station.pid == station->getPid())
        continue;
        
      if (station->debug)
        std::cout << "a message was received" << std::endl;

      inet_ntop(AF_INET, &(client_addr.sin_addr), client_data.station.ipAddress, INET_ADDRSTRLEN);
      Station participant = Station::deserialize(client_data.station);

      struct message msg;
      msg.type = client_data.type;
      msg.address = client_addr.sin_addr.s_addr;
      msg.payload = participant;
      msg.sequence = client_data.seqn;

      switch (client_data.subservice)
      {
        case DISCOVERY:
          datagram_queue->mutex_discovery.lock();
          datagram_queue->discovery_queue.push_back(msg);
          datagram_queue->mutex_discovery.unlock();
          break;

        case MONITORING:
        default:
          datagram_queue->mutex_monitoring.lock();
          datagram_queue->monitoring_queue.push_back(msg);
          datagram_queue->mutex_monitoring.unlock();
      }
    }

    n = recvfrom(sockfd, &client_replicate_data, sizeof(struct table_packet), 0, (struct sockaddr *) &client_addr, &client_addr_len);
    if (n > 0)
    {   
      if (station->debug)
        std::cout << "a message was received" << std::endl;

      /*StationTable participant =*/ 
      StationTable station_table;
      station_table.deserialize(client_replicate_data.station_table);
      /*
      struct replicate_message msg;
      msg.type = client_replicate_data.type;
      msg.address = client_addr.sin_addr.s_addr;
      msg.payload = participant;
      msg.sequence = client_replicate_data.seqn;

      datagram_queue->mutex_replicate.lock();
      datagram_queue->replicate_queue.push_back(msg);
      datagram_queue->mutex_replicate.unlock();*/
    }
  }

  close(sockfd);
  return 0;
}

struct packet datagram::create_packet(MessageType type, short sequence, 
    struct station_serial payload)
{
  struct packet packet;
  if (type == STATUS_REQUEST || type == STATUS_RESPONSE)
    packet.subservice = MONITORING;
  else
    packet.subservice = DISCOVERY;
  packet.type = type;
  packet.seqn = sequence;
  packet.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch() ).count();
  packet.length = sizeof(payload);
  packet.station = payload;
  return packet;
}

struct table_packet datagram::create_replicate_packet(MessageType type, short sequence, 
    struct station_table_serial payload)
{
  struct table_packet packet;
  packet.subservice = REPLICATE;
  packet.type = type;
  packet.seqn = sequence;
  packet.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch() ).count();
  packet.length = sizeof(payload);
  packet.station_table = payload;
  return packet;
}

int datagram::open_socket()
{
  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    std::cerr << "ERROR opening socket" << std::endl;
      
  // struct timeval timeout;
  // timeout.tv_sec = 0;
  // timeout.tv_usec = 500000; // 500ms
  // int ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  // if (ret < 0)
  //   std::cout << "ERROR option timeout" << std::endl;
      
  int broadcastEnable = 1;
  int ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
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
