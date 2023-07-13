#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "sleep_server.h"

namespace interface {
  
  void *server (Station* station);
  void *client (Station* station);

};

#endif