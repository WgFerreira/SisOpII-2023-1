#ifndef _MANAGEMENT_H
#define _MANAGEMENT_H

#include "sleep_server.h"

namespace management {
  
  void *server (Station* station);
  void *client (Station* station);

};

#endif