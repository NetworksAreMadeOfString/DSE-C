/*
* Copyright (C) 2011 - Gareth Llewellyn
*
* This file is part of DSE-c - http://blog.NetworksAreMadeOfString.co.uk/DSE/
*
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License along with
* this program. If not, see <http://www.gnu.org/licenses/>
*/
#include <stdio.h>
#include <string.h>
#include "sbus.h"
#include "dse.h"
using namespace std;

int main(int argc, char* argv[])
{
        //First things first is to check for command line arguements
        while ((cmdLineArg = getopt (argc, argv, "vhp:m:Dd")) != -1)
        {
                switch (cmdLineArg)
                {
                        case 'v': printf("DataSift Embedded Installation Device-0.1b\n"); return 0;
                        case 'h': printf("If you don't know check the source code\n"); break;
                        case 'p': pin = atoi(optarg); break;
                        case 'm': mode = atoi(optarg); break;
                        case 'D': daemonMode = 1; break;
                        case 'd': printf("Enabling Debugging output...\n"); debug=1; break;
                        case '?': fprintf (stderr, "Option -%c requires an argument.\n", optopt); break;
                        default: return 1;
                }
        }

	if(getConfig() == 2)
	{
		return 2;
	}
	else
	{
		if(debug == 1)
			printf("Hash: %s | User: %s | APIKey %s\n",StreamHash,UserID,APIKey);
	}

	if(daemonMode == 1)
	{
		if(debug == 1)
	                printf("DataSift Embedded - Starting in daemon mode\n");

		//Get stream hash from config
		while(runasDaemon() != 99)
		{
			if(debug == 1)
				printf("99 means a change in the config file was found - reload and restart");

			//Get updated stream hash
			getConfig();
		}
	}
	else
	{
		if(pin == 26 || pin < 17)
        	{
                	printf("Sorry %i is not a valid pin choice\n",pin);
			return 2;
        	}

        	if(mode > 1)
        	{
                	mode = 0;
        	}

        	printf("Setting %i to %i\n",pin,mode);
		sbuslock();
		setdiopin(pin, mode);
		sbusunlock();
	}
	return 0;
}
