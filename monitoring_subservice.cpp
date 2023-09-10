#include <iostream>
#include <string>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <map>
#include <thread>

#include "include/monitoring_subservice.h"
#include "include/sleep_server.h"

using namespace datagram;
using namespace management;

void *monitoring::monitor (Station* station, DatagramQueue *datagram_queue, ManagementQueue *manage_queue, StationTable *table)
{
  while (station->status != EXITING)
  {
    /**
     * Envia mensagem de monitoramento
     */
    if (station->getType() == MANAGER)
    {
      std::list<datagram::message> messages;
      std::list<management::station_op_data> operations;

      if (table->mutex_write.try_lock())
      {
        for (auto &participant : table->getValues(0))
        {
          if (participant.macAddress == station->macAddress)
            continue;

          if (millis_since(participant.last_update) > station->monitor_interval 
              && millis_since(participant.last_update_request) > station->monitor_interval)
          {
            if (participant.update_request_retries > 2 && participant.status != ASLEEP)
            {
              if (station->debug)
                std::cout << "monitor: Um participante parou de responder -> ASLEEP" << std::endl;
              struct management::station_op_data op;
              op.operation = ManagerOperation::UPDATE_STATUS;
              op.key = participant.macAddress;
              op.new_status = ASLEEP;

              operations.push_back(op);
            }
              
            struct message request_msg;
            request_msg.address = participant.getAddress();
            request_msg.sequence = 0;
            request_msg.type = STATUS_REQUEST;
            request_msg.payload = *station;

            messages.push_back(request_msg);
            participant.update_request_retries += 1;
            participant.last_update_request = now();
          }
        }
        table->mutex_write.unlock();

        if (!operations.empty())
        {
          if (station->debug)
            std::cout << "monitor: Atualizando status de " << messages.size() << " participantes para ASLEEP." << std::endl;
          manage_queue->mutex_manage.lock();
          manage_queue->manage_queue.splice(manage_queue->manage_queue.end(), operations);
          manage_queue->mutex_manage.unlock();
        }

        if (!messages.empty())
        {
          if (station->debug)
            std::cout << "monitor: Requisitando status de " << messages.size() << " participantes." << std::endl;
          datagram_queue->mutex_sending.lock();
          datagram_queue->sending_queue.splice(datagram_queue->sending_queue.end(), messages);
          datagram_queue->mutex_sending.unlock();
        }
      }
    }

    /**
     * Processa mensagens da fila
    */
    if (!datagram_queue->monitoring_queue.empty()) {
      datagram_queue->mutex_monitoring.lock();

      struct message msg = datagram_queue->monitoring_queue.front();
      datagram_queue->monitoring_queue.pop_front();
      datagram_queue->mutex_monitoring.unlock();
      
      switch (msg.type)
      {
      case MessageType::STATUS_REQUEST : 
      {
        if (station->debug)
          std::cout << "monitor: Respondendo requisição de status" << std::endl;
        struct message response_msg;
        response_msg.address = msg.address;
        response_msg.sequence = 0;
        response_msg.type = STATUS_RESPONSE;
        response_msg.payload = *station;

        datagram_queue->mutex_sending.lock();
        datagram_queue->sending_queue.push_back(response_msg);
        datagram_queue->mutex_sending.unlock();

        station->last_update = now();
      }
        break;
        
      case MessageType::STATUS_RESPONSE :
        if (station->getType() == MANAGER)
        {
          if (station->debug)
            std::cout << "monitor: Recebeu resposta de um participante" << std::endl;
          struct management::station_op_data op;
          op.operation = ManagerOperation::UPDATE_STATUS;
          op.key = msg.payload.macAddress;
          op.new_status = AWAKEN;

          manage_queue->mutex_manage.lock();
          manage_queue->manage_queue.push_back(op);
          manage_queue->mutex_manage.unlock();
        }
        break;
      
      default:
        break;
      }
    }

    /**
     * Monitora o Manager
    */
    if (station->getType() == PARTICIPANT && station->getManager() != NULL)
    {
      if (millis_since(station->last_update) >= station->monitor_interval*1.5)
      {
        if (station->debug)
          std::cout << "monitor: Manager parou de requisitar status" << std::endl;
        station->setManager(NULL);
      }
    }
  }

  if (station->debug)
    std::cout << "saindo monitor" << std::endl;
  return 0;
}
