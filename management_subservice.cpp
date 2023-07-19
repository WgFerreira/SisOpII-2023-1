#include <iostream>
#include <unistd.h>
#include <string.h>
#include <semaphore>
#include <iterator>
#include <list>

#include "include/management_subservice.h"
#include "include/sleep_server.h"

using namespace std;

void *management::manageHostTable (Station* station) 
{
    while(station->status != EXITING) 
    {	
    	smphSignalManagToPrint.release();
    	smphSignalPrintToManag.acquire();

        smphAccessHostTable.acquire();

        if (station_buffer.operation == INSERT)
            stations_table.insert(std::pair<std::string,Station>(station_buffer.key, station_buffer.station));
        else 
        if (station_buffer.operation == DELETE)
            stations_table.erase(station_buffer.key);
        else 
        if (station_buffer.operation == UPDATE_STATUS)
            stations_table[station_buffer.key].status = station_buffer.new_status;

        smphAccessStationBuffer.release();
    
    	if(station->getType() == StationType::MANAGER){
    	    smphSignalManagToDiscoveryHostTable.release();
    	    smphSignalDiscoveryToManagHostTable.acquire();
    	    
    	    list_of_stations.push_back(hostTable);
    	    
    	    
	    smphSignalManagToMonitoringHostTable.release();
	    smphSignalMonitoringToManagHostTable.acquire();
	    
	    
	    //management::update_station_status();
	     
    	} else {
    	    smphSignalManagToDiscoverySetManager.release();
    	    smphSignalDiscoveryToManagSetManager.acquire();
    	}
    
    }
    
}

/*
void *management::update_station_status () 
{
    list<Station>::iterator it;
    it = std::find(list_of_stations.begin(), list_of_stations.end(), hostTable);
    
    //if (it != list_of_stations.end()) {
    //    it->status = hostTable.status;
    //} else {
        cout << "Erro to find hostTable" << endl;
    //}
}
*/
