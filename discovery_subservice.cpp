#include "include/discovery_subservice.h"

#include <iostream>
// #include <cstring>
// #include <chrono>
// #include <unistd.h>
// #include <sys/socket.h>
// #include <arpa/inet.h>

// #include "include/sleep_server.h"

void *discovery::discovery (Station* station, MessageQueue *send_queue, 
        MessageQueue *discovery_queue, OperationQueue *manage_queue, 
        management::StationTable *table)
{
    discovery_queue->mutex_read.lock();
    while (station->status != EXITING)
    {
        discovery_queue->mutex_read.lock();
        /**
         * Processa mensagens da fila
        */
        while (!discovery_queue->queue.empty()) {
            struct message msg = discovery_queue->pop();

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

                    send_queue->push(victory_msg);
                    
                    /**
                     * Se o remetente não é conhecido, então é precisa ser adicionada a tabela
                    */
                    struct table_operation op;
                    op.operation = INSERT;
                    op.key = msg.payload.macAddress;
                    op.station = msg.payload;

                    manage_queue->push(op);
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
                        mutex_no_manager.unlock();
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

                            send_queue->push(answer_msg);
                        }
                        /**
                         * Se essa é a estação com pid mais alto, 
                         * é provavelmente o novo líder
                        */
                        else 
                            election_victory(station, send_queue, table);
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
                mutex_no_manager.lock();
                table->mutex_read.unlock();

                station->last_update = now();
                break;

            case MessageType::LEAVING :
                if (station->getType() == MANAGER)
                {
                    if (station->debug)
                        std::cout << "discovery: Uma estação está saindo da rede" << std::endl;
                    if (table->has(msg.payload.macAddress)) 
                    {
                        table_operation op;
                        op.operation = DELETE;
                        op.key = msg.payload.macAddress;

                        manage_queue->push(op);
                    }
                }
                else 
                {
                    if (station->debug)
                        std::cout << "discovery: O Manager está saindo da rede" << std::endl;
                    if (msg.payload.macAddress == station->getManager()->macAddress) {
                        station->setManager(NULL);
                        mutex_no_manager.unlock();
                    }
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

    send_queue->push(exit_msg);

    if (station->debug)
        std::cout << "discovery: saindo" << std::endl;
    return 0;
}

void *discovery::election(Station* station, MessageQueue *send_queue, management::StationTable *table)
{
    while (station->status != EXITING)
    {
        mutex_no_manager.lock();
        mutex_no_manager.unlock();
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
            leader_election(station, send_queue, table);

        /**
         * Se já perdeu a eleição, mas ainda nao tem resposta, inicia a eleição de novo
        */
        if (station->status == WAITING_ELECTION && millis_since(station->last_leader_search) > station->election_timeout)
            station->status = ELECTING;
    }

    if (station->debug)
        std::cout << "election: saindo" << std::endl;
    return 0;
}

void discovery::leader_election(Station* station, MessageQueue *send_queue, management::StationTable *table)
{
    /**
     * Se é a terceira vez que tenta iniciar a eleição, então não teve resposta e a estação está eleita
    */
    if (station->leader_search_retries >= 2)
    {
        if (station->debug)
            std::cout << "discovery: Nenhuma Reposta na Eleição" << std::endl;
        election_victory(station, send_queue, table);
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

            send_queue->push(election_msg);
        }
        else {
            if (station->debug)
                std::cout << "discovery: Multicasting eleição" << std::endl;
            multicast_election(station, send_queue, table, MANAGER_ELECTION, true);
        }
            
        station->last_leader_search = now();
        station->leader_search_retries += 1;
        station->status = ELECTING;
    }
}

void discovery::election_victory(Station* station, MessageQueue *send_queue, management::StationTable *table)
{
    station->last_leader_search = now();
    station->leader_search_retries = 0;
    station->setType(MANAGER);

    station->setManager(NULL);
    mutex_no_manager.lock();
    table->mutex_read.unlock();

    station->status = AWAKEN;

    if (table->table.empty()) {
        if (station->debug)
            std::cout << "discovery: Broadcasting vitória" << std::endl;
        struct message victory_msg;
        victory_msg.address = INADDR_BROADCAST;
        victory_msg.sequence = 0;
        victory_msg.type = ELECTION_VICTORY;
        victory_msg.payload = *station;
        
        send_queue->push(victory_msg);
    }
    else {
        if (station->debug)
            std::cout << "discovery: Multicasting vitória" << std::endl;
        multicast_election(station, send_queue, table, ELECTION_VICTORY, false); 
    }
}


void discovery::multicast_election(Station* station, MessageQueue *send_queue, 
        management::StationTable *table, MessageType type, bool filter_pid)
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

    send_queue->push(messages);
}

