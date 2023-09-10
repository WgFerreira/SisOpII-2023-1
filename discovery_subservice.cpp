#include <iostream>
#include <cstring>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "include/discovery_subservice.h"
#include "include/sleep_server.h"

using namespace datagram;
using namespace management;

void *discovery::discovery (Station* station, datagram::DatagramQueue *datagram_queue, management::ManagementQueue *manage_queue, management::StationTable *table)
{
    while (station->status != EXITING)
    {
        /**
         * Checa se tem manager e inicia a eleição se necessário
        */
        bully_algorithm(station, datagram_queue, table);

        /**
         * Processa mensagens da fila
        */
        if (!datagram_queue->discovery_queue.empty()) {
            datagram_queue->mutex_discovery.lock();

            struct message msg = datagram_queue->discovery_queue.front();
            datagram_queue->discovery_queue.pop_front();
            datagram_queue->mutex_discovery.unlock();

            switch (msg.type)
            {
            case MessageType::MANAGER_ELECTION :
                /**
                 * Se é o manager, então não houve falha e não é necessário eleger um manager novo
                 * só responder com vitória
                */
                if (station->getType() == MANAGER)
                {
                    if (station->debug)
                        std::cout << "discovery: Manager recebeu mensagem de eleição" << std::endl;
                    struct message victory_msg;
                    victory_msg.address = msg.address;
                    victory_msg.sequence = 0;
                    victory_msg.type = ELECTION_VICTORY;
                    victory_msg.payload = *station;

                    datagram_queue->mutex_sending.lock();
                    datagram_queue->sending_queue.push_back(victory_msg);
                    datagram_queue->mutex_sending.unlock();
                    
                    /**
                     * Se o remetente não é conhecido, então é precisa ser adicionada a tabela
                    */
                    if (table->has(msg.payload.macAddress))
                    {
                        struct management::station_op_data op;
                        op.operation = INSERT;
                        op.key = msg.payload.macAddress;
                        op.station = msg.payload;

                        manage_queue->mutex_manage.lock();
                        manage_queue->manage_queue.push_back(op);
                        manage_queue->mutex_manage.unlock();
                    }
                }
                /**
                 * Se é só um participante, talvez tenha que iniciar uma eleição
                */
                else {
                    /**
                     * Se o remetente é conhecido, houve uma falha
                     * continua o processo de eleição 
                    */
                    if (table->has(msg.payload.macAddress))
                    {
                        if (station->debug)
                            std::cout << "discovery: Participante recebeu mensagem de eleição" << std::endl;
                        station->setManager(NULL);
                        /**
                         * Se existem outras estação conhecidas com o pid mais alto, 
                         * então responde e continua a eleição
                        */
                        if (table->getValues(station->getPid()).size() > 0 || station->getPid() > msg.payload.getPid()) 
                        {
                            struct message answer_msg;
                            answer_msg.address = msg.address;
                            answer_msg.sequence = 0;
                            answer_msg.type = ELECTION_ANSWER;
                            answer_msg.payload = *station;

                            datagram_queue->mutex_sending.lock();
                            datagram_queue->sending_queue.push_back(answer_msg);
                            datagram_queue->mutex_sending.unlock();
                        }
                        /**
                         * Se essa é a estação com pid mais alto, 
                         * é provavelmente o novo líder
                        */
                        else 
                            election_victory(station, datagram_queue, table);
                    }
                }
                break;
                
            case MessageType::ELECTION_ANSWER :
                if (station->debug)
                    std::cout << "discovery: Perdeu eleição" << std::endl;
                station->setType(PARTICIPANT);
                station->status = WAITING_ELECTION;
                station->last_leader_search = now();
                station->leader_search_retries = 0;
                break;
                
            case MessageType::ELECTION_VICTORY :
                if (station->debug)
                    std::cout << "discovery: Novo Manager Recebido" << std::endl;
                station->setType(PARTICIPANT);
                station->status = AWAKEN;
                station->last_leader_search = now();
                station->leader_search_retries = 0;
                station->setManager(&msg.payload);
                station->last_update = now();
                break;

            case MessageType::LEAVING :
                if (station->getType() == MANAGER)
                {
                    if (station->debug)
                        std::cout << "discovery: Uma estação está saindo da rede" << std::endl;
                    if (table->has(msg.payload.macAddress)) 
                    {
                        struct management::station_op_data op;
                        op.operation = DELETE;
                        op.key = msg.payload.macAddress;

                        manage_queue->mutex_manage.lock();
                        manage_queue->manage_queue.push_back(op);
                        manage_queue->mutex_manage.unlock();
                    }
                }
                else 
                {
                    if (station->debug)
                        std::cout << "discovery: O Manager está saindo da rede" << std::endl;
                    if (msg.payload.macAddress == station->getManager()->macAddress)
                        station->setManager(NULL);
                }
                break;
            
            default:
                break;
            }
        }
    }

    struct message exit_msg;
    exit_msg.address = INADDR_BROADCAST;
    exit_msg.sequence = 0;
    exit_msg.type = LEAVING;
    exit_msg.payload = *station;

    datagram_queue->mutex_sending.lock();
    datagram_queue->sending_queue.push_back(exit_msg);
    datagram_queue->mutex_sending.unlock();

    if (station->debug)
        std::cout << "saindo discovery" << std::endl;
    return 0;
}

void discovery::bully_algorithm(Station* station, datagram::DatagramQueue *datagram_queue, management::StationTable *table)
{
    /**
     * Se não tem manager, inicia eleição
    */
    if (station->getType() != MANAGER && station->getManager() == NULL)
        station->status = ELECTING;

    /**
     * Se está em eleição e ainda não tem resposta, tenta de novo 
     *  ou termina a eleição com vitória
    */
    if (station->status == ELECTING && millis_since(station->last_leader_search) > station->election_timeout)
        leader_election(station, datagram_queue, table);

    /**
     * Se já perdeu a eleição, mas ainda nao tem resposta, inicia a eleição de novo
    */
    if (station->status == WAITING_ELECTION && millis_since(station->last_leader_search) > station->election_timeout)
        station->status = ELECTING;
}

void discovery::leader_election(Station* station, datagram::DatagramQueue *datagram_queue, management::StationTable *table)
{
    /**
     * Se é a terceira vez que tenta iniciar a eleição, então não teve resposta e a estação está eleita
    */
    if (station->leader_search_retries >= 2)
    {
        if (station->debug)
            std::cout << "discovery: Nenhuma Reposta na Eleição" << std::endl;
        election_victory(station, datagram_queue, table);
    } 
    /**
     * Pode tentar iniciar uma eleição até 2 vezes seguidas
    */
    else {
        if (station->debug)
            std::cout << "discovery: Iniciando Eleição" << std::endl;
        station->setType(CANDIDATE);

        if (table->table.empty()) {
            if (station->debug)
                std::cout << "discovery: Broadcasting eleição" << std::endl;
            struct message election_msg;
            election_msg.address = INADDR_BROADCAST;
            election_msg.sequence = 0;
            election_msg.type = MANAGER_ELECTION;
            election_msg.payload = *station;

            datagram_queue->mutex_sending.lock();
            datagram_queue->sending_queue.push_back(election_msg);
            datagram_queue->mutex_sending.unlock();
        }
        else {
            if (station->debug)
                std::cout << "discovery: Multicasting eleição" << std::endl;
            multicast_election(station, datagram_queue, table, MANAGER_ELECTION, true);
        }
            
        station->last_leader_search = now();
        station->leader_search_retries += 1;
        station->status = ELECTING;
    }
}

void discovery::multicast_election(Station* station, datagram::DatagramQueue *datagram_queue, management::StationTable *table, datagram::MessageType type, bool filter_pid)
{
    std::list<struct message> messages;

    table->mutex_write.lock();
    for (auto &s : table->getValues(filter_pid ? station->getPid() : 0)) {
        if (s.getAddress() == station->getAddress())
            continue;

        struct message election_msg;
        election_msg.address = s.getAddress();
        election_msg.sequence = 0;
        election_msg.type = type;
        election_msg.payload = *station;
        
        messages.push_back(election_msg);
    }
    table->mutex_write.unlock();

    datagram_queue->mutex_sending.lock();
    datagram_queue->sending_queue.splice(datagram_queue->sending_queue.end(), messages);
    datagram_queue->mutex_sending.unlock();
}

void discovery::election_victory(Station* station, datagram::DatagramQueue *datagram_queue, management::StationTable *table)
{
    station->last_leader_search = now();
    station->leader_search_retries = 0;
    station->setType(MANAGER);
    station->setManager(NULL);
    station->status = AWAKEN;

    std::list<struct message> victory_messages;
    if (table->table.empty()) {
        if (station->debug)
            std::cout << "discovery: Broadcasting vitória" << std::endl;
        struct message victory_msg;
        victory_msg.address = INADDR_BROADCAST;
        victory_msg.sequence = 0;
        victory_msg.type = ELECTION_VICTORY;
        victory_msg.payload = *station;
        
        datagram_queue->mutex_sending.lock();
        datagram_queue->sending_queue.push_back(victory_msg);
        datagram_queue->mutex_sending.unlock();
    }
    else {
        if (station->debug)
            std::cout << "discovery: Multicasting vitória" << std::endl;
        multicast_election(station, datagram_queue, table, ELECTION_VICTORY, false); 
    }
}

