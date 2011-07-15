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

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


//Custom libraries
#include "sbus.h"
#include "dse.h"

int core = 0;
int main(int argc, char* argv[])
{
        //First things first is to check for command line arguements
        //It's very rare for ought to be used other than -d whilst I'm
        //developing it.
        while ((cmdLineArg = getopt (argc, argv, "vhs:c:d")) != -1)
        {
                switch (cmdLineArg)
                {
                        case 'v': printf("DataSift Embedded Installation Device-0.1b\n"); return 0;
                        case 'h': printf("If you don't know check the source code\n"); break;
                        case 's': score = atoi(optarg); break;
			case 'c': core = atoi(optarg); break;
                        //case 'l': strncpy(location,optarg,25); break;
                        case 'd': printf("Enabling Debugging output...\n"); debug=1; break;
                        case '?': fprintf (stderr, "Option -%c requires an argument.\n", optopt); break;
                        default: return 1;
                }
        }

        if(debug == 1)
		printf("Starting an LED hunt....%i\n",score);

	//score = 500;
	//core = 2000;
	int i = 0;

	sbuslock();

	setdiopin(25, 0);
	sleep(5);
	//setdiopin(25, 2);
	//sleep(5);
	setdiopin(25, 1);


	sbusunlock();

	//We should never get here!
        printf("Exiting gracefully\n");
        //0 means all is OK! :)
        return 0;
}
