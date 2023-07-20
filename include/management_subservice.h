#ifndef _MANAGEMENT_H
#define _MANAGEMENT_H

#include "sleep_server.h"

namespace management {
  
  /**
   * Não entendi o que tem que fazer aqui
  */
  void *manageHostTable(Station* station, StationTable* table, struct semaphores *sem);
  
  //void *update_station_status();

};

#endif
