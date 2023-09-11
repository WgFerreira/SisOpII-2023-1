#include <iostream>
#include <iomanip>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <bits/stdc++.h>
#include <semaphore>
#include <iterator>
#include <list>

#include "include/interface_subservice.h"
#include "include/sleep_server.h"

using namespace std;

void *interface::interface (Station* station, StationTable* table)
{
	const char separator    = ' ';
	const int nameWidth     = 30;

	while(station->atomic_GetStatus() != EXITING) 
	{
		table->mutex_read.lock();
		if (!station->debug)
		{
			struct winsize w;
			ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

			system("clear");

			cout << endl;
			cout << endl;
			cout << left << setw(3) << setfill(separator) << " ";
			cout << left << setw(nameWidth) << setfill(separator) << "HOSTNAME";
			cout << left << setw(20) << setfill(separator) << "MAC ADDRESS";
			cout << left << setw(20) << setfill(separator) << "IP ADDRESS";
			cout << left << setw(10) << setfill(separator) << "STATUS";
			cout << endl;
			cout << "--------------------------------------------------------------------------------------------------------------";
			cout << endl;
			
			station->print_interface();

			for (auto &tupla : table->table)
			{
				Station s = tupla.second;
				s.print_interface();
			}
		}
		else 
		{
			station->print_interface();
			cout << "interface: table has update " << table->has_update << endl;
		}
		table->mutex_read.unlock();
    table->mutex_read.lock();
    table->has_update = false;
	}
	if (station->debug)
		std::cout << "saindo interface" << std::endl;
	return 0;
}

void *interface::command (Station* station, StationTable* table) 
{
	string command_values[5];
	
	while(station->atomic_GetStatus() != EXITING) {
		string command;
		getline(cin, command);
		
		stringstream ss(command);
		string word;
		
		int pointer = 0;
		while (ss >> word) {
			command_values[pointer] = word;
			pointer++;
		}
		
		if (station->atomic_GetType() == StationType::MANAGER) 
		{
			if (command_values[0].compare("wakeup") == 0) 
			{
				string macAddress = "";
				table->mutex_write.lock();
				for (auto &tupla : table->table)
				{
					if (tupla.second.GetHostname() == command_values[1])
					{
						macAddress = tupla.second.GetMacAddress();
						break;
					}
				}
				table->mutex_write.unlock();

				if (macAddress.size() > 0)
				{
					stringstream cmd;
					cmd << "wakeonlan " << macAddress;
					system(cmd.str().c_str());
				}
			}
		}
		
		if (command_values[0].compare("EXIT") == 0)
		{
			station->atomic_SetStatus(EXITING);
			end_all_threads_safely();
		}
		table->has_update = true;
	}
	if (station->debug)
		std::cout << "command: saindo" << std::endl;
	return 0;
}
