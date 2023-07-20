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

void *interface::printServer (Station* station, StationTable* table, struct semaphores *sem) {
	const char separator    = ' ';
	const int nameWidth     = 30;

	while(station->status != EXITING) {
		if (table->has_update)
		{
			sem->mutex_read.lock();
		
			system("clear");

			cout << endl;
			cout << endl;
			cout << left << setw(nameWidth) << setfill(separator) << "HOSTNAME";
			cout << left << setw(nameWidth) << setfill(separator) << "MAC ADDRESS";
			cout << left << setw(nameWidth) << setfill(separator) << "IP ADDRESS";
			cout << left << setw(nameWidth) << setfill(separator) << "STATUS";
			cout << endl;
			cout << "--------------------------------------------------------------------------------------------------------------";
			cout << endl;

			for (auto &tupla : table->table)
			{
				Station s = tupla.second;
				if (s.hostname.length() > 0)
				{
					string status = "";
					if (s.status == AWAKEN)
						status = "AWAKEN";
					else
						status = "ASLEEP";
					
					cout << left << setw(nameWidth) << setfill(separator) << s.hostname;
					cout << left << setw(nameWidth) << setfill(separator) << s.macAddress;
					cout << left << setw(nameWidth) << setfill(separator) << s.ipAddress;
					cout << left << setw(nameWidth) << setfill(separator) << status;
					cout << endl;
				}
			}
			
			table->has_update = false;
			sem->mutex_read.unlock();
		}
	}
}

void *interface::printClient (Station* station, StationTable* table, struct semaphores *sem) {
	const char separator    = ' ';
	const int nameWidth     = 30;

	while(station->status != EXITING) {
		if (table->has_update)
		{
			sem->mutex_manager.lock();
			system("clear");

			cout << endl;
			cout << endl;
			cout << left << setw(nameWidth) << setfill(separator) << "HOSTNAME";
			cout << left << setw(nameWidth) << setfill(separator) << "MAC ADDRESS";
			cout << left << setw(nameWidth) << setfill(separator) << "IP ADDRESS";
			cout << left << setw(nameWidth) << setfill(separator) << "STATUS";
			cout << endl;
			cout << "--------------------------------------------------------------------------------------------------------------";
			cout << endl;
			
			if (station->getManager() != NULL) {
				string status = "";
				if (station->getManager()->status == AWAKEN)
					status = "AWAKEN";
				else
					status = "ASLEEP";
				
				cout << left << setw(nameWidth) << setfill(separator) << station->getManager()->hostname;
				cout << left << setw(nameWidth) << setfill(separator) << station->getManager()->macAddress;
				cout << left << setw(nameWidth) << setfill(separator) << station->getManager()->ipAddress;
				cout << left << setw(nameWidth) << setfill(separator) << status;
			}
			cout << endl;

			table->has_update = false;
			sem->mutex_manager.unlock();
		}
	}
}


void *interface::getCommand (Station* station, StationTable* table, struct semaphores *sem) {
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
		
		if (station->getType() == StationType::MANAGER) 
		{
			if (command_values[0].compare("wakeup") == 0) 
			{
				string macAddress = "";
				sem->mutex_read.lock();
				for (auto &tupla : table->table)
				{
					if (tupla.second.hostname.compare(command_values[1]))
					{
						macAddress = tupla.second.macAddress;
						break;
					}
				}
				sem->mutex_read.unlock();

				if (macAddress.size() > 0)
				{
					stringstream cmd;
					cmd << "wakeonlan " << macAddress;
					system(cmd.str().c_str());
				}
				table->has_update = true;
			}
		}
		
		if (command_values[0].compare("EXIT") == 0)
		{
			station->status = EXITING;
			table->has_update = true;
		}
	}
}
