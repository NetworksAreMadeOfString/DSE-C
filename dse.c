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
