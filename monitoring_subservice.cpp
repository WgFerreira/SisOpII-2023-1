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

void *monitoring::monitor_request (Station* station, MessageQueue *send_queue, 
    OperationQueue *manage_queue, StationTable *table)
{
  while (station->atomic_GetStatus() != EXITING)
  {
    /**
     * Envia mensagem de monitoramento
     */
    if (station->atomic_GetType() == MANAGER)
    {
      std::list<message> messages;
      std::list<table_operation> operations;

      auto list = table->getValues(0);
        if (station->debug)
          std::cout << "monitor: " << list.size() << " participantes para monitorar" << std::endl;
      for (auto participant : list)
      {
        if (participant.GetMacAddress() == station->GetMacAddress())
          continue;

        if (participant.update_request_retries > 2 && participant.GetStatus() != ASLEEP)
        {
          if (station->debug)
            std::cout << "monitor: Um participante parou de responder -> ASLEEP" << std::endl;
          table_operation op;
          op.operation = TableOperation::UPDATE_STATUS;
          op.key = participant.GetMacAddress();
          op.new_status = ASLEEP;
          op.new_type = participant.GetType();

          operations.push_back(op);
        }
          
        struct message request_msg;
        request_msg.address = participant.getAddress();
        request_msg.sequence = 0;
        request_msg.type = STATUS_REQUEST;
        request_msg.station = *station;

        messages.push_back(request_msg);

        table_operation op;
        op.operation = TableOperation::UPDATE_RETRY;
        op.key = participant.GetMacAddress();

        operations.push_back(op);

        if (station->debug)
          std::cout << "monitor: tentou " << participant.update_request_retries << " vezes" << std::endl;
      }

      if (!operations.empty())
      {
        if (station->debug)
          std::cout << "monitor: Atualizando status de " << messages.size() << " participantes." << std::endl;
        manage_queue->push(operations);
      }

      if (!messages.empty())
      {
        if (station->debug)
          std::cout << "monitor: Requisitando status de " << messages.size() << " participantes." << std::endl;
        send_queue->push(messages);
      }
    }

    /**
     * Monitora o Manager
    */
    auto interval = station->atomic_GetMonitor_interval();
    if (station->atomic_GetType() == PARTICIPANT && station->atomic_GetManager() != NULL)
    {
      auto millis_update = millis_since(station->atomic_GetLast_update());
      if (millis_update >= interval*1.5)
      {
        if (station->debug)
          std::cout << "monitor: Manager parou de requisitar status a " << millis_update << " ms" << std::endl;
        station->atomic_SetManager(NULL);
        mutex_no_manager.unlock();
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
  }

  if (station->debug)
    std::cout << "monitor request: saindo" << std::endl;
  return 0;
}

void *monitoring::monitor_respond (Station* station, MessageQueue *send_queue, 
    MessageQueue *monitor_queue, OperationQueue *manage_queue)
{
  monitor_queue->mutex_read.lock();
  while (station->atomic_GetStatus() != EXITING)
  {
    monitor_queue->mutex_read.lock();
    /**
     * Processa mensagens da fila
    */
    while (!monitor_queue->queue.empty()) {
      struct message msg = monitor_queue->pop();
      
      Station payload = msg.station;

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
        response_msg.station = *station;

        send_queue->push(response_msg);

        station->atomic_SetLast_update(now());
      }
        break;
        
      case MessageType::STATUS_RESPONSE :
        if (station->atomic_GetType() == MANAGER)
        {
          if (station->debug)
            std::cout << "monitor: Recebeu resposta de um participante" << std::endl;
          table_operation op;
          op.operation = TableOperation::UPDATE_STATUS;
          op.key = payload.GetMacAddress();
          op.new_status = payload.GetStatus();
          op.new_type = payload.GetType();

          manage_queue->push(op);
        }
        break;
      
      default:
        break;
      }
    }
  }

  if (station->debug)
    std::cout << "monitor response: saindo" << std::endl;
  return 0;
}
