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

void *interface::interface (Station* station, management::StationTable* table)
{
	const char separator    = ' ';
	const int nameWidth     = 30;

	while(station->status != EXITING) 
	{
		if (table->mutex_read.try_lock())
		{
			if (!station->debug)
			{
				struct winsize w;
				ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

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
			}
			else 
			{
				// station->printStation();
				cout << "interface: table has update " << table->has_update << endl;
			}
			table->mutex_read.unlock();
		}
	}
	if (station->debug)
		std::cout << "saindo interface" << std::endl;
	return 0;
}

void *interface::command (Station* station, management::StationTable* table) 
{
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
				table->mutex_write.lock();
				for (auto &tupla : table->table)
				{
					if (tupla.second.hostname.compare(command_values[1]))
					{
						macAddress = tupla.second.macAddress;
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
			station->status = EXITING;
		}
		table->has_update = true;
	}
	if (station->debug)
		std::cout << "saindo command" << std::endl;
	return 0;
}
