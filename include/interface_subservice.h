#ifndef _INTERFACE_H
#define _INTERFACE_H

#include <unistd.h>
#include <string.h>

#include "management_subservice.h"
#include "station_table.h"
#include "sleep_server.h"

namespace interface {
  
  /**
   * Apresenta tabela de hosts apara a estação líder, ou informações da
   * estação líder para a estação participante
  */
  void *interface (Station* station, StationTable* table);

  /**
   * Espera comando EXIT ou WAKEUP host
  */
  void *command (Station* station, StationTable* table);

};

#endif
