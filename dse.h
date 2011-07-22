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
#include <linux/input.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>   /* Standard input/output definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <curl/curl.h>
#include <iostream>
#include <fstream>
using namespace std;

#define DIO_Z 2
#define VERSION "0.1b"
#define lvl1 25 //no such thing as lvl1 as there aren't enough DIO ports
#define lvl2 25
#define lvl3 24
#define lvl4 23
#define lvl5 22
#define lvl6 21
#define lvl7 20
#define lvl8 19
#define lvl9 18
#define lvl10 17

//Various ints used for return codes debugging bools etc
int debug = 0, pin = 0, mode = 0, toggle = 0,  cmdLineArg = 0, daemonMode = 0;

//Threads
pthread_t CountDownThread;

//Thread return codes
int countDownRC;

//Where we are
char location[128];

//The DataSift stream hash id
char StreamHash[33];
char UserID[50];
char APIKey[33];

//Function prototypes
void ProcessScore(int Score);

int getConfig()
{
	ifstream configFile;
	configFile.open("/etc/dse.conf");
 
	if ( !configFile )
  	{
		return 2;
  	} 

	char _buff[1024], _ch=' ', tag[24], confValue[50];

	while( !configFile.eof() )
	{
        	/*_ch = configFile.get();
        	if(_ch != '#' && _ch != '\n')
		{
            		configFile.getline(_buff,1024);
            		//puts(_buff);
            		sscanf(_buff,"%s %*s %s",tag,confValue);
            		printf("Tag: [%c]%s Value: %s\n",_ch,tag,confValue);
        	}*/

		configFile.getline(_buff,1024);
		if(_buff[0] != '#' && _buff[0] != '\n' && _buff[0] != ' ')
		{
			sscanf(_buff,"%s %*s %s",tag,confValue);

			if(debug == 1)
                        	printf("Tag: %s Value: %s\n",tag,confValue);

			if(strcmp(tag,"hash") == 0)
				strncpy(StreamHash,confValue,32);

			if(strcmp(tag,"userid") == 0)
                                strcpy(UserID,confValue);

			if(strcmp(tag,"apikey") == 0)
                                strncpy(APIKey,confValue,32);
		}

        	//configFile.ignore(1024,'\n');
        	_ch = configFile.peek();
        	while(_ch==' ' && _ch=='\n')
		{
            		configFile.ignore(1024,'\n');
            		_ch = configFile.peek();
        	}
    	}
    	configFile.close();


	//This will always return 0 if we got some data as
	//by default I'll use a public stream and a 0 userid	
    	return 0;
}

/*******************************************************************************
* setdiopin: accepts a DIO register and value to place in that DIO pin.
*   Values can be 0 (low), 1 (high), or 2 (z - high impedance).
* Was part of sbus.c but has since gone. Probably still (c) Technologic Systems
*******************************************************************************/
void setdiopin(int pin, int val)
{
   int pinOffSet;
   int dirPinOffSet; // For Register 0x66 only
   int outPinOffSet; // For Register 0x66 only

   // First, check for the high impedance case
   if (val == 2)
   {
      if (pin <= 40 && pin >= 37)
      {
         dirPinOffSet = pin - 33;
         sbus_poke16(0x66, sbus_peek16(0x66) & ~(1 << dirPinOffSet));
      }
      else if (pin <= 36 && pin >= 21)
      {
         pinOffSet = pin - 21;
         sbus_poke16(0x6c, sbus_peek16(0x6c) & ~(1 << pinOffSet));
      }
      else if (pin <= 20 && pin >= 5)
      {
         pinOffSet = pin - 5;
         sbus_poke16(0x72, sbus_peek16(0x72) & ~(1 << pinOffSet));
      }
   }

   /******************************************************************* 
   *0x66: DIO and tagmem control (RW)
   *  bit 15-12: DIO input for pins 40(MSB)-37(LSB) (RO)
   *  bit 11-8: DIO output for pins 40(MSB)-37(LSB) (RW)
   *  bit 7-4: DIO direction for pins 40(MSB)-37(LSB) (1 - output) (RW)
   ********************************************************************/
   else if (pin <= 40 && pin >= 37)
   {
      dirPinOffSet = pin - 33; // -37 + 4 = Direction; -37 + 8 = Output
      outPinOffSet = pin - 29;

      // set bit [pinOffset] to [val] of register [0x66] 
      if(val)
         sbus_poke16(0x66, (sbus_peek16(0x66) | (1 << outPinOffSet)));
      else
         sbus_poke16(0x66, (sbus_peek16(0x66) & ~(1 << outPinOffSet)));

      // Make the specified pin into an output in direction bits
      sbus_poke16(0x66, sbus_peek16(0x66) | (1 << dirPinOffSet)); ///

   }

   /********************************************************************* 
   *0x68: DIO input for pins 36(MSB)-21(LSB) (RO)    
   *0x6a: DIO output for pins 36(MSB)-21(LSB) (RW)
   *0x6c: DIO direction for pins 36(MSB)-21(LSB) (1 - output) (RW)
   *********************************************************************/
   else if (pin <= 36 && pin >= 21)
   {
      pinOffSet = pin - 21;

      // set bit [pinOffset] to [val] of register [0x6a] 
      if(val)
         sbus_poke16(0x6a, (sbus_peek16(0x6a) | (1 << pinOffSet)));
      else
         sbus_poke16(0x6a, (sbus_peek16(0x6a) & ~(1 << pinOffSet)));

      // Make the specified pin into an output in direction register
      sbus_poke16(0x6c, sbus_peek16(0x6c) | (1 << pinOffSet)); ///
   }

   /********************************************************************* 
   *0x6e: DIO input for pins 20(MSB)-5(LSB) (RO)    
   *0x70: DIO output for pins 20(MSB)-5(LSB) (RW)
   *0x72: DIO direction for pins 20(MSB)-5(LSB) (1 - output) (RW)
   *********************************************************************/
   else if (pin <= 20 && pin >= 5)
   {
      pinOffSet = pin - 5;

      if(val)
         sbus_poke16(0x70, (sbus_peek16(0x70) | (1 << pinOffSet)));
      else
         sbus_poke16(0x70, (sbus_peek16(0x70) & ~(1 << pinOffSet)));

      // Make the specified pin into an output in direction register
      sbus_poke16(0x72, sbus_peek16(0x72) | (1 << pinOffSet));
   }

}



/* Auxiliary function that waits on the socket. */
static int wait_on_socket(curl_socket_t sockfd, int for_recv, long timeout_ms)
{
  struct timeval tv;
  fd_set infd, outfd, errfd;
  int res;

  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec= (timeout_ms % 1000) * 1000;

  FD_ZERO(&infd);
  FD_ZERO(&outfd);
  FD_ZERO(&errfd);

  FD_SET(sockfd, &errfd); /* always check for error */

  if(for_recv)
  {
    FD_SET(sockfd, &infd);
  }
  else
  {
    FD_SET(sockfd, &outfd);
  }

  /* select() returns the number of signalled sockets or -1 */
  res = select(sockfd + 1, &infd, &outfd, &errfd, &tv);
  return res;
}


int runasDaemon(void)
{
	CURL *curl;
  	CURLcode res;
  	//const char *request = "GET /5465fa0683583e96b21a1faddbca4ea4?username=NetworkString&api_key=xxxxxxxxxxxxxxxxxxxxxxxxx HTTP/1.1\r\nHost: stream.datasift.net\r\n\r\n";
	
	char request2[256];
	strcpy(request2,"GET /");
	strncat(request2,StreamHash,32);
	strcat(request2,"?username=");
	strncat(request2,UserID,strlen(UserID));
	strcat(request2,"&api_key=");
	strncat(request2,APIKey,32);
	strcat(request2," HTTP/1.1\r\nHost: stream.datasift.net\r\n\r\n");

  	curl_socket_t sockfd; /* socket */
  	long sockextr;
  	size_t iolen;

  	curl = curl_easy_init();
  	if(curl) 
	{
    		curl_easy_setopt(curl, CURLOPT_URL, "http://stream.datasift.net");
    		/* Do not do the transfer - only connect to host */
    		curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
    		res = curl_easy_perform(curl);

    		if(CURLE_OK != res)
   	 	{
      			printf("Error: %s\n", strerror(res));
      			return 1;
    }

    /* Extract the socket from the curl handle - we'll need it for waiting.
 *      * Note that this API takes a pointer to a 'long' while we use
 *           * curl_socket_t for sockets otherwise.
 *                */
    res = curl_easy_getinfo(curl, CURLINFO_LASTSOCKET, &sockextr);

    if(CURLE_OK != res)
    {
      printf("Error: %s\n", curl_easy_strerror(res));
      return 1;
    }

    sockfd = sockextr;

    /* wait for the socket to become ready for sending */
    if(!wait_on_socket(sockfd, 0, 60000L))
    {
      printf("Error: timeout.\n");
      return 1;
    }
   puts("Sending request.");
    /* Send the request. Real applications should check the iolen
 *      * to see if all the request has been sent */
    res = curl_easy_send(curl, request2, strlen(request2), &iolen);

    if(CURLE_OK != res)
    {
      printf("Error: %s\n", curl_easy_strerror(res));
      return 1;
    }
    puts("Reading response.");

    /* read the response */
    for(;;)
    {
      char buf[4096];

      wait_on_socket(sockfd, 1, 60000L);
      res = curl_easy_recv(curl, buf, 4096, &iolen);

      if(CURLE_OK != res)
        break;

      //printf("Received %u bytes. (%s)\n", iolen,buf);
        if(buf[5] == '{')
        {
                char interaction[iolen+1];
                strncpy(interaction,buf,iolen);
                interaction[iolen+1] = '\0';

                char * kloutPointer;
                kloutPointer = strstr (interaction,"score");
                if(kloutPointer != NULL)
                {
			//I'm sure there are **far** more elegant ways of doing this
			//but hey
			char temp[11];
			char strScore[3];
			int intScore = 0;
			strncpy(temp,kloutPointer,10);
			strScore[0] = temp[7];
			if(temp[8] != ',' && temp[8] != '}')
				strScore[1] = temp[8];

			if(temp[9] != ',' && temp[9] != '}')
                                strScore[2] = temp[9];

			if(temp[10] != ',' && temp[10] != '}')
                                strScore[3] = temp[10];

			intScore = atoi(strScore);

			if(debug == 1)
	                	printf("Found a Klout Score of %i\n",intScore);

			//Change the LEDs
			ProcessScore(intScore);
                }
                else
                {
			if(debug == 1)
	                        printf("No Klout Score Found!\n");
                }

		//This isn't needed anymore
                //printf("||||||||||| %s ||||||||||\n",interaction);
        }
        else
        {
		if(debug == 1)
	                printf("Not a valid start of interaction\n");
        }

	//I want to put a check in here to detect if the stream hash 
	//has changed and then exit the loop with a return code of say 99 
	//go back to main, see the return code grab the new hash and 
	//restart the stream - for now it's infinite loop time
    }

    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  return 0;
}


void ProcessScore(int Score)
{
	if(debug == 1)
		printf("Received a score of %i\n",Score);

	sbuslock();
	//Lazy
	if(Score < 10)
	{
		setdiopin(lvl2,0);
                setdiopin(lvl3,0);
                setdiopin(lvl4,0);
                setdiopin(lvl5,0);
                setdiopin(lvl6,0);
                setdiopin(lvl7,0);
                setdiopin(lvl8,0);
                setdiopin(lvl9,0);
                setdiopin(lvl10,0);
	}
	else if(Score >= 10 && Score < 20)
	{
		setdiopin(lvl2,1);
		setdiopin(lvl3,0);
		setdiopin(lvl4,0);
		setdiopin(lvl5,0);
		setdiopin(lvl6,0);
		setdiopin(lvl7,0);
		setdiopin(lvl8,0);
		setdiopin(lvl9,0);
		setdiopin(lvl10,0);
		
	}
	else if(Score >= 20 && Score < 30)
	{
		setdiopin(lvl2,1);
                setdiopin(lvl3,1);
                setdiopin(lvl4,0);
                setdiopin(lvl5,0);
                setdiopin(lvl6,0);
                setdiopin(lvl7,0);
                setdiopin(lvl8,0);
                setdiopin(lvl9,0);
                setdiopin(lvl10,0);
	}
	else if(Score >= 30 && Score < 40)
        {
                setdiopin(lvl2,1);
                setdiopin(lvl3,1);
                setdiopin(lvl4,1);
                setdiopin(lvl5,0);
                setdiopin(lvl6,0);
                setdiopin(lvl7,0);
                setdiopin(lvl8,0);
                setdiopin(lvl9,0);
                setdiopin(lvl10,0);
        }
	else if(Score >= 40 && Score < 50)
        {
                setdiopin(lvl2,1);
                setdiopin(lvl3,1);
                setdiopin(lvl4,1);
                setdiopin(lvl5,1);
                setdiopin(lvl6,0);
                setdiopin(lvl7,0);
                setdiopin(lvl8,0);
                setdiopin(lvl9,0);
                setdiopin(lvl10,0);
        }
	else if(Score >= 50 && Score < 60)
        {
                setdiopin(lvl2,1);
                setdiopin(lvl3,1);
                setdiopin(lvl4,1);
                setdiopin(lvl5,1);
                setdiopin(lvl6,1);
                setdiopin(lvl7,0);
                setdiopin(lvl8,0);
                setdiopin(lvl9,0);
                setdiopin(lvl10,0);
        }
	else if(Score >= 60 && Score < 70)
        {
                setdiopin(lvl2,1);
                setdiopin(lvl3,1);
                setdiopin(lvl4,1);
                setdiopin(lvl5,1);
                setdiopin(lvl6,1);
                setdiopin(lvl7,1);
                setdiopin(lvl8,0);
                setdiopin(lvl9,0);
                setdiopin(lvl10,0);
        }
	else if(Score >= 70 && Score < 80)
        {
                setdiopin(lvl2,1);
                setdiopin(lvl3,1);
                setdiopin(lvl4,1);
                setdiopin(lvl5,1);
                setdiopin(lvl6,1);
                setdiopin(lvl7,1);
                setdiopin(lvl8,1);
                setdiopin(lvl9,0);
                setdiopin(lvl10,0);
        }
	else if(Score >= 80 && Score < 90)
        {
                setdiopin(lvl2,1);
                setdiopin(lvl3,1);
                setdiopin(lvl4,1);
                setdiopin(lvl5,1);
                setdiopin(lvl6,1);
                setdiopin(lvl7,1);
                setdiopin(lvl8,1);
                setdiopin(lvl9,1);
                setdiopin(lvl10,0);
        }
	else if(Score >= 90)
	{
		setdiopin(lvl2,1);
		setdiopin(lvl3,1);
		setdiopin(lvl4,1);
		setdiopin(lvl5,1);
		setdiopin(lvl6,1);
		setdiopin(lvl7,1);
		setdiopin(lvl8,1);
		setdiopin(lvl9,1);
		setdiopin(lvl10,1);
	}
	else
	{
		setdiopin(25,0);
	}
	sbusunlock();
}
