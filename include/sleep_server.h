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

    //public: 
    void init(std::string arg);
};

#endif
