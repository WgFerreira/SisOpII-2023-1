#ifndef _SERVER_H
#define _SERVER_H

#include <string>
#include <limits.h>

#define PORT 5555

enum StationType : uint8_t 
{
    MANAGER,
    PARTICIPANT
};

class Station 
{
public:
    StationType type = PARTICIPANT;
    char macAddress[18];
    char hostname[HOST_NAME_MAX];

    void init(std::string arg);
    void extractMacAddress();
};

enum StationStatus : uint8_t 
{
    AWAKEN,
    ASLEEP
};

struct station_entry 
{
    std::string hostName;
    std::string ipAddress;
    std::string macAddress;
    StationStatus status;
};

extern struct station_entry hostTable;

enum PacketType: uint16_t 
{
    DATA_PACKET,
    CMD_PACKET
};

struct packet 
{
    PacketType type; //Tipo do pacote (p.ex. DATA | CMD)
    uint16_t seqn; //Número de sequência
    uint16_t length; //Comprimento do payload
    uint64_t timestamp; // Timestamp do dado
    Station station;
    char _payload[255]; //Dados da mensagem
};

#endif
