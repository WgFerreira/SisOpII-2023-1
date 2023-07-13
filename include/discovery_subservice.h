#ifndef _DISCOVERY_H
#define _DISCOVERY_H

#include "sleep_server.h"
namespace discovery {
  
  void *server (Station* station);
  void *client (Station* station);

};


#endif