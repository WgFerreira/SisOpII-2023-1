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
        StationTable *table)
{
    discovery_queue->mutex_read.lock();
    while (station->atomic_GetStatus() != EXITING)
    {
        discovery_queue->mutex_read.lock();
        /**
         * Processa mensagens da fila
        */
        while (!discovery_queue->queue.empty()) {
            struct message msg = discovery_queue->pop();
            
            Station payload = msg.station;

            switch (msg.type)
            {
            case MessageType::MANAGER_ELECTION :
                /**
                 * Se é o manager, então não houve falha e não é necessário eleger um manager novo
                 * só responder com vitória
                */
                if (station->atomic_GetType() == MANAGER)
                {
                    if (station->debug)
                        std::cout << "discovery: Manager recebeu mensagem de eleição" << std::endl;
                    struct message victory_msg;
                    victory_msg.address = msg.address;
                    victory_msg.sequence = 0;
                    victory_msg.type = ELECTION_VICTORY;
                    victory_msg.station = *station;

                    send_queue->push(victory_msg);
                    
                    /**
                     * Se o remetente não é conhecido, então é precisa ser adicionada a tabela
                    */
                    struct table_operation op;
                    op.operation = INSERT;
                    op.key = payload.GetMacAddress();
                    op.station = payload;

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
                    
                    if (table->has(payload.GetMacAddress()) && station->atomic_GetStatus() != WAITING_ELECTION)
                    {

                    if (station->debug)
                        std::cout << "discovery: Participante recebeu mensagem de eleição" << std::endl;
                    mutex_no_manager.unlock();
                    station->atomic_SetManager(NULL);
                    struct table_operation op;
                    op.operation = INSERT;
                    op.key = station->GetMacAddress();
                    op.station = *station;
                    manage_queue->push(op);
                    /**
                     * Se existem outras estação conhecidas com o pid mais alto, 
                     * então responde e continua a eleição
                    */
                    if (station->GetPid() > payload.GetPid()) 
                    {
                        struct message answer_msg;
                        answer_msg.address = msg.address;
                        answer_msg.sequence = 0;
                        answer_msg.type = ELECTION_ANSWER;
                        answer_msg.station = *station;

                        send_queue->push(answer_msg);
                    }
                    /**
                     * Se essa é a estação com pid mais alto, 
                     * é provavelmente o novo líder
                    */
                        if (table->getValues(station->GetPid()).empty()) 
                    	    election_victory(station, send_queue, manage_queue, table);
		    }
                }
                break;
                
            case MessageType::ELECTION_ANSWER :
                if (station->atomic_GetStatus() == ELECTING)
                {
                    if (station->debug)
                        std::cout << "discovery: Perdeu eleição" << std::endl;
                    station->atomic_set([](Station* self) {
                        self->SetType(PARTICIPANT);
                        self->SetStatus(WAITING_ELECTION);
                        self->SetLast_leader_search(now());
                        self->SetLeader_search_retries(0);
                    });
                }
                break;
                
            case MessageType::ELECTION_VICTORY :
            {
                if (station->debug)
                    std::cout << "discovery: Novo Manager Recebido" << std::endl;
                mutex_no_manager.lock();
                station->atomic_set([&](Station* self) {
                    self->SetType(PARTICIPANT);
                    self->SetStatus(AWAKEN);
                    self->SetLast_leader_search(now());
                    self->SetLeader_search_retries(0);
                    self->SetLast_update(now());
                    self->SetManager(&payload);
                });
                struct table_operation op;
                op.operation = INSERT;
                op.key = station->GetMacAddress();
                op.station = *station;
                manage_queue->push(op);
                break;
            }

            case MessageType::LEAVING :
                if (station->atomic_GetType() == MANAGER)
                {
                    if (station->debug)
                        std::cout << "discovery: Uma estação está saindo da rede" << std::endl;
                    table_operation op;
                    op.operation = DELETE;
                    op.key = payload.GetMacAddress();

                    manage_queue->push(op);
                }
                else 
                {
                    if (station->debug)
                        std::cout << "discovery: O Manager está saindo da rede" << std::endl;
                    if (payload.GetMacAddress() == station->atomic_GetManager()->GetMacAddress()) {
                        station->atomic_SetManager(NULL);
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
    exit_msg.station = *station;

    send_queue->push(exit_msg);

    if (station->debug)
        std::cout << "discovery: saindo" << std::endl;
    return 0;
}

void *discovery::election(Station* station, MessageQueue *send_queue, OperationQueue *manage_queue, StationTable *table)
{
    while (station->atomic_GetStatus() != EXITING)
    {
        mutex_no_manager.lock();
        mutex_no_manager.unlock();
        /**
         * Se não tem manager, inicia eleição
        */
        if (station->atomic_GetType() != MANAGER && station->atomic_GetManager() == NULL)
            station->atomic_SetStatus(ELECTING);

        /**
         * Se está em eleição e ainda não tem resposta, tenta de novo 
         *  ou termina a eleição com vitória
        */
        uint64_t timeout = station->atomic_GetElection_timeout();
        uint64_t last_search = station->atomic_GetLast_leader_search();
        if (station->atomic_GetStatus() == ELECTING && millis_since(last_search) > timeout)
            leader_election(station, send_queue, manage_queue, table);

        /**
         * Se já perdeu a eleição, mas ainda nao tem resposta, inicia a eleição de novo
        */
        if (station->atomic_GetStatus() == WAITING_ELECTION && millis_since(last_search) > timeout)
            station->atomic_SetStatus(ELECTING);
            
    }

    if (station->debug)
        std::cout << "election: saindo" << std::endl;
    return 0;
}

void discovery::leader_election(Station* station, MessageQueue *send_queue, OperationQueue *manage_queue, StationTable *table)
{
    /**
     * Se é a terceira vez que tenta iniciar a eleição, então não teve resposta e a estação está eleita
    */
    if (station->atomic_GetLeader_search_retries() >= 2)
    {
        if (station->debug)
            std::cout << "discovery: Nenhuma Reposta na Eleição" << std::endl;
        election_victory(station, send_queue, manage_queue, table);
    } 
    /**
     * Pode tentar iniciar uma eleição até 2 vezes seguidas
    */
    else {
        if (station->debug)
            std::cout << "discovery: Iniciando Eleição" << std::endl;
        station->atomic_SetType(CANDIDATE);

        auto list = table->getValues(0);
        list.remove_if([&](Station &s) { return s.GetPid() == station->GetPid(); });
        if (list.size() <= 2) {
            if (station->debug)
                std::cout << "discovery: Broadcasting eleição" << std::endl;
            struct message election_msg;
            election_msg.address = INADDR_BROADCAST;
            election_msg.sequence = 0;
            election_msg.type = MANAGER_ELECTION;
            election_msg.station = *station;

            send_queue->push(election_msg);
        }
        else {
            if (station->debug)
                std::cout << "discovery: Multicasting eleição" << std::endl;
            multicast_election(station, send_queue, manage_queue, table, MANAGER_ELECTION, true);
        }
        
        station->atomic_set([](Station *self) {
            self->SetLast_leader_search(now());
            self->SetLeader_search_retries(self->GetLeader_search_retries() + 1);
            // self->SetStatus(ELECTING);
        });
    }
}

void discovery::election_victory(Station* station, MessageQueue *send_queue, OperationQueue *manage_queue, StationTable *table)
{
    mutex_no_manager.lock();
    station->atomic_set([](Station *self) {
        self->SetLast_leader_search(now());
        self->SetLeader_search_retries(0);
        self->SetType(MANAGER);
        self->SetStatus(AWAKEN);
        self->SetManager(NULL);
    });

    struct table_operation op;
    op.operation = INSERT;
    op.key = station->GetMacAddress();
    op.station = *station;
    manage_queue->push(op);

    auto list = table->getValues(0);
    list.remove_if([&](Station &s) { return s.GetPid() == station->GetPid(); });
    if (list.size() <= 2) {
        if (station->debug)
            std::cout << "discovery: Broadcasting vitória" << std::endl;
        struct message victory_msg;
        victory_msg.address = INADDR_BROADCAST;
        victory_msg.sequence = 0;
        victory_msg.type = ELECTION_VICTORY;
        victory_msg.station = *station;
        
        send_queue->push(victory_msg);
    }
    else {
        if (station->debug)
            std::cout << "discovery: Multicasting vitória" << std::endl;
        multicast_election(station, send_queue, manage_queue, table, ELECTION_VICTORY, false); 
    }
}


void discovery::multicast_election(Station* station, MessageQueue *send_queue, OperationQueue *manage_queue, 
        StationTable *table, MessageType type, bool filter_pid)
{
    std::list<struct message> messages;

    auto list = table->getValues(filter_pid ? station->GetPid() : 0);
    list.remove_if([&](Station &s) { return s.GetPid() == station->GetPid(); });
    table->mutex_write.lock();
    for (auto &s : list) {
        // if (s.GetMacAddress() == station->GetMacAddress())
        //     continue;

        struct message election_msg;
        election_msg.address = s.getAddress();
        election_msg.sequence = 0;
        election_msg.type = type;
        election_msg.station = *station;
        
        messages.push_back(election_msg);
    }
    table->mutex_write.unlock();
    
    send_queue->push(messages);
}

