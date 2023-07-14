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
    std::string macAddress;

    void init(std::string arg);

private:
    std::string extractMacAddress();
};

enum PacketType: uint16_t {
    DATA_PACKET,
    CMD_PACKET
};

struct packet 
{
    PacketType type; //Tipo do pacote (p.ex. DATA | CMD)
    uint16_t seqn; //Número de sequência
    uint16_t length; //Comprimento do payload
    uint64_t timestamp; // Timestamp do dado
    char _payload[255]; //Dados da mensagem
};

#endif
