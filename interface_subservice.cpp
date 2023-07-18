#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <string.h>
#include <bits/stdc++.h>
#include <semaphore>
#include <iterator>
#include <list>

#include "include/interface_subservice.h"
#include "include/sleep_server.h"

using namespace std;

void *interface::print (Station* station) {
    const char separator    = ' ';
    const int nameWidth     = 30;

    while(station->status != EXITING) {
    	smphSignalManagToPrint.acquire();
    	
    	cout << endl;
    	cout << endl;
    	cout << left << setw(nameWidth) << setfill(separator) << "HOSTNAME";
    	cout << left << setw(nameWidth) << setfill(separator) << "MAC ADDRESS";
    	cout << left << setw(nameWidth) << setfill(separator) << "IP ADDRESS";
    	cout << left << setw(nameWidth) << setfill(separator) << "STATUS";
    	cout << endl;
    	cout << "--------------------------------------------------------------------------------------------------------------";
    	cout << endl;
    
    	if (station->getType() == StationType::MANAGER) {
    	
    	    list<Station>::iterator it;
	    for (it = list_of_stations.begin(); it != list_of_stations.end(); ++it) {
	        
    	        cout << left << setw(nameWidth) << setfill(separator) << it->hostname;
    	        cout << left << setw(nameWidth) << setfill(separator) << it->macAddress;
    	        cout << left << setw(nameWidth) << setfill(separator) << it->ipAddress;
	        cout << left << setw(nameWidth) << setfill(separator) << it->status;
    	        cout << endl;
    	    
    	    }
    
    	} else {
    
    	    if (station->getManager() != NULL) {
    	    
    	        cout << left << setw(nameWidth) << setfill(separator) << station->getManager()->hostname;
    	        cout << left << setw(nameWidth) << setfill(separator) << station->getManager()->macAddress;
    	        cout << left << setw(nameWidth) << setfill(separator) << station->getManager()->ipAddress;
	        cout << left << setw(nameWidth) << setfill(separator) << station->getManager()->status;
	    
	    }
    	    cout << endl;
    
       	}
       	
        smphSignalPrintToManag.release();
    }
}


void *interface::getCommand (Station* station) {
    string command_values[5];
    
    while(station->status != EXITING) {
        string command;
        getline(cin, command);
        
        stringstream ss(command);
        string word;
        
        int pointer = 0;
        while (ss >> word) {
            command_values[pointer] = word;
            pointer++;
        }
        
        if (station->getType() == StationType::MANAGER) {
            if (command_values[0].compare("wakeup") == 0) {
                interface::wakeonlan(command_values[1]);
                command_values[1] = "\0";
            }
        } else {
            if (command_values[0].compare("EXIT") == 0) {
                interface::exit_service(station);
            }
        }
    }

}


void *interface::wakeonlan(string mac_address) {
    cout << "WAKEONLAN COMMAND " << mac_address << endl;
}


void *interface::exit_service(Station* station) {
    cout << "EXIT SERVICE COMMAND " << station->hostname << endl;
    
    int sockfd = open_socket();

    while (station->status != EXITING)
    {    
        if (station->getManager() != NULL)
        {
            struct sockaddr_in sock_addr = station->getManager()->getSocketAddress();
            
            cout << sock_addr.sin_family << endl;
            cout << sock_addr.sin_port << endl;
            cout << sock_addr.sin_addr.s_addr << endl;
        
            struct packet data = create_packet(SLEEP_SERVICE_EXITING, 0, "Sleep Service Exiting");
            data.station = station->getManager()->serialize();

            int n = sendto(sockfd, &data, sizeof(data), 0, (struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in));
            if (n < 0)
                cout << "ERROR sendto : monitor exit" << endl;
            else
            	station->status = EXITING;
	
        }
    }
    
    close(sockfd);
    return 0;
}
