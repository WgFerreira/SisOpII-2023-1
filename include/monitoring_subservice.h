#ifndef _MONITOR_H
#define _MONITOR_H

#include "sleep_server.h"
namespace monitoring {
  
  void *server (Station* station);
  void *client (Station* station);

};

#endif