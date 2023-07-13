#ifndef _SERVER_H
#define _SERVER_H

#include <string>

#define PORT 5555

enum StationType {
    MANAGER,
    PARTICIPANT
};

class Station 
{
    public:
    StationType type = PARTICIPANT;
    int address;

    void init(std::string arg);
};

enum PacketType: uint16_t {
    DATA_PACKET,
    CMD_PACKET
};
struct __packet {
    PacketType type; //Tipo do pacote (p.ex. DATA | CMD)
    uint16_t seqn; //Número de sequência
    uint16_t length; //Comprimento do payload
    uint16_t timestamp; // Timestamp do dado
    const char* _payload; //Dados da mensagem
};

#endif
