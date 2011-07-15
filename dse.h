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




#define DIO_Z 2
#define VERSION "0.1b"
#define lvl1 25
#define lvl2 23
#define lvl3 22
#define lvl4 21
#define lvl5 20
#define lvl6 19
#define lvl7 18
#define lvl8 17
#define lvl9 16
#define lvl10 15

//Various ints used for return codes debugging bools etc
int debug = 0, score = 0, secs = 0, toggle = 0,  cmdLineArg = 0;

//Threads
pthread_t CountDownThread;

//Thread return codes
int countDownRC;

//Where we are
char location[128];

