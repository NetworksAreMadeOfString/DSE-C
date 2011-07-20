/* Copyright 2010, Unpublished Work of Technologic Systems
 * All Rights Reserved.
 *
 * THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL,
 * PROPRIETARY AND TRADE SECRET INFORMATION OF TECHNOLOGIC SYSTEMS.
 * ACCESS TO THIS WORK IS RESTRICTED TO (I) TECHNOLOGIC SYSTEMS 
 * EMPLOYEES WHO HAVE A NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE
 * OF THEIR ASSIGNMENTS AND (II) ENTITIES OTHER THAN TECHNOLOGIC
 * SYSTEMS WHO HAVE ENTERED INTO APPROPRIATE LICENSE AGREEMENTS.  NO
 * PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED, COPIED, 
 * DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED, CONDENSED, 
 * EXPANDED, COLLECTED, COMPILED, LINKED, RECAST, TRANSFORMED, ADAPTED
 * IN ANY FORM OR BY ANY MEANS, MANUAL, MECHANICAL, CHEMICAL, 
 * ELECTRICAL, ELECTRONIC, OPTICAL, BIOLOGICAL, OR OTHERWISE WITHOUT
 * THE PRIOR WRITTEN PERMISSION AND CONSENT OF TECHNOLOGIC SYSTEMS.
 * ANY USE OR EXPLOITATION OF THIS WORK WITHOUT THE PRIOR WRITTEN
 * CONSENT OF TECHNOLOGIC SYSTEMS COULD SUBJECT THE PERPETRATOR TO
 * CRIMINAL AND CIVIL LIABILITY.
 */

/* Version history:
 *
 * 01/01/2011 - KB
 *   Added includes and a #define to be compatible with TEMP_FAILURE_RETRY
 *
 * 12/30/2010 - KB
 *   Added TEMP_FAILURE_RETRY to sbuslock()
 *
 * 12/16/2010 - KB
 *   Updated reservemem() to properly check if we have access to the map
 *   Included win* functions for memwindow with TS-4500
 *   Included sbus_*stream16 functions to read a buffer of specified lenght
 *	from the SBUS mem window
 *   Comment blocks for function information 
 *
 * 04/08/2011 - KB
 *   Added assert()'s to fail if any SBUS transactions are tried without 
 *	previously locking the SBUS
 *
 * 04/13/2011 - KB for JO
 *   Modified sbus_poke16() to correct arm9 handling of short vs long
 *   
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define MW_ADR    0x18
#define MW_CONF   0x1a
#define MW_DAT1   0x1c
#define MW_DAT2   0x1e
#if !defined(_GNU_SOURCE)
	#define _GNU_SOURCE
#endif

#ifndef TEMP_FAILURE_RETRY
# define TEMP_FAILURE_RETRY(expression) \
  (__extension__                                                              \
    ({ long int __result;                                                     \
       do __result = (long int) (expression);                                 \
       while (__result == -1L && errno == EINTR);                             \
       __result; }))
#endif



void sbus_poke16(unsigned int, unsigned short);
unsigned short sbus_peek16(unsigned int);

void winpoke16(unsigned int, unsigned short);
unsigned short winpeek16(unsigned int);
void winpoke32(unsigned int, unsigned int);
unsigned int winpeek32(unsigned int);
void winpoke8(unsigned int, unsigned char);
unsigned char winpeek8(unsigned int);

void sbus_peekstream16(int adr_reg, int dat_reg, int adr, unsigned char *buf, int n);
void sbus_pokestream16(int adr_reg, int dat_reg, int adr, unsigned char *buf, int n);

void sbuslock(void);
void sbusunlock(void);
void sbuspreempt(void); 

static volatile unsigned int *cvspiregs, *cvgpioregs;
static int last_gpio_adr = 0;
static int sbuslocked = 0;

/* The following SBUS peek16/poke16 functions are the base for all other SBUS
 * read/write functions.  These are written in assembly in order to optimize
 * these functions as best as possible.  The SBUS is 16-bits wide and all 
 * transactions and registers are based on this.  It is possible to do 8 and 32
 * bit transactions by using shifts or combining two of these functions back to
 * back, hence they were not included in the SBUS API for simplicities sake.
 * 
 * The sbus_poke16 function takes an adr and a dat, it will write the dat to 
 * the location specified by adr.
 *
 * The sbus_peek16 function only takes an adr argument and returns a short of
 * the data located at adr.
 */

void sbus_poke16(unsigned int adr, unsigned short dat) {
	unsigned int dummy = 0;
	unsigned int d = dat;
	assert(sbuslocked == 1);
	if (last_gpio_adr != adr >> 5) {
		last_gpio_adr = adr >> 5;
		cvgpioregs[0] = (cvgpioregs[0] & ~(0x3<<15))|((adr>>5)<<15);
	}
	adr &= 0x1f;

	asm volatile (
		"mov %0, %1, lsl #18\n"
		"orr %0, %0, #0x800000\n"
		"orr %0, %0, %2, lsl #3\n"
		"3: ldr r1, [%3, #0x64]\n"
		"cmp r1, #0x0\n"
		"bne 3b\n"
		"2: str %0, [%3, #0x50]\n"
		"1: ldr r1, [%3, #0x64]\n"
		"cmp r1, #0x0\n"
		"beq 1b\n"
		"ldr %0, [%3, #0x58]\n"
		"ands r1, %0, #0x1\n"
		"moveq %0, #0x0\n"
		"beq 3b\n"
		: "+r"(dummy) : "r"(adr), "r"(d), "r"(cvspiregs) : "r1","cc"
	);
}


unsigned short sbus_peek16(unsigned int adr) {
	unsigned short ret = 0;
	assert(sbuslocked == 1);

	if (last_gpio_adr != adr >> 5) {
		last_gpio_adr = adr >> 5;
		cvgpioregs[0] = ((adr>>5)<<15|1<<3|1<<17);
	}
	adr &= 0x1f;

	asm volatile (
		"mov %0, %1, lsl #18\n"
		"2: str %0, [%2, #0x50]\n"
		"1: ldr r1, [%2, #0x64]\n"
		"cmp r1, #0x0\n"
		"beq 1b\n"
		"ldr %0, [%2, #0x58]\n"
		"ands r1, %0, #0x10000\n"
		"bicne %0, %0, #0xff0000\n"
		"moveq %0, #0x0\n"
		"beq 2b\n" 
		: "+r"(ret) : "r"(adr), "r"(cvspiregs) : "r1", "cc"
	);

	return ret;

}

/* The win* functions are designed for use with the TS-4500 using the opencore
 * and any baseboard that supports memory windowing. This current includes any
 * external bus, baseboard ADC support, or any custom baseboard features.
 *
 * The opencore wb_memwindow.v describes the layout and register mapping. The
 * opencore has support for 8, 16, and 32bit bus cycles so we provide functions
 * for them here.
 *
 * These functions work exactly the same as the sbus_*16 functions.  The poke
 * version requires both adr and dat and return nothing.  The peek version 
 * requires adr and returns a data type equal to its bit-width.
 */

void winpoke16(unsigned int adr, unsigned short dat) {
	assert(sbuslocked == 1);

        sbus_poke16(MW_ADR, adr >> 11);
        sbus_poke16(MW_CONF, (adr & 0x7ff) | (0x10 << 11));
        sbus_poke16(MW_DAT1, dat);
}

void xwinpoke16(unsigned int adr, unsigned short dat) {
	assert(sbuslocked == 1);

        sbus_poke16(MW_ADR, adr >> 11);
        //sbus_poke16(MW_CONF, (adr & 0x7ff) | (0x10 << 11));
        //sbus_poke16(MW_DAT1, dat);
}

unsigned short winpeek16(unsigned int adr) {
	assert(sbuslocked == 1);

        sbus_poke16(MW_ADR, adr >> 11);
        sbus_poke16(MW_CONF, (adr & 0x7ff) | (0x10 << 11));
        return sbus_peek16(MW_DAT1);
}

void winpoke32(unsigned int adr, unsigned int dat) {
	assert(sbuslocked == 1);

        sbus_poke16(MW_ADR, adr >> 11);
        sbus_poke16(MW_CONF, (adr & 0x7ff) | (0x0d << 11));
        sbus_poke16(MW_DAT2, dat & 0xffff);
        sbus_poke16(MW_DAT2, dat >> 16);
}

unsigned int winpeek32(unsigned int adr) {
	assert(sbuslocked == 1);

        unsigned int ret;

        sbus_poke16(MW_ADR, adr >> 11);
        sbus_poke16(MW_CONF, (adr & 0x7ff) | (0x0d << 11));
        ret = sbus_peek16(MW_DAT2);
        ret |= (sbus_peek16(MW_DAT2) << 16);
        return ret;
}

void winpoke8(unsigned int adr, unsigned char dat) {
	assert(sbuslocked == 1);
        sbus_poke16(MW_ADR, adr >> 11);
        sbus_poke16(MW_CONF, (adr & 0x7ff) | (0x18 << 11));
        sbus_poke16(MW_DAT1, dat);
}

unsigned char winpeek8(unsigned int adr) {
	assert(sbuslocked == 1);
        sbus_poke16(MW_ADR, adr >> 11);
        sbus_poke16(MW_CONF, (adr & 0x7ff) | (0x18 << 11));
        return sbus_peek16(MW_DAT1) & 0xff;
}

/* The sbus_*stream16 functions are meant to stream data over the SBUS.
 * Intended to be used with memory windows in the FPGA, it is not normally
 * used with the default FPGA functionality and meant to be useful to customers
 * who wish to include a memory window in the FPGA using the internal blockram.
 * 
 * The opencore wb_memwindow.v describes the register layout and the
 * layout of these two functions.
 *
 * The arguments to sbus_peekstream16() are dat_reg which is the location of the
 * data register, adr_reg is the location of the address register of the mem
 * window, adr is the address to start the peek stream from, buf is a pointer to
 * a buffer to write the data to, and n is length of data to read.
 * 
 * The arguments to sbus_pokestream16() are dat_reg which is the location of the
 * data register, adr_reg is the location of the address register of the mem
 * window, adr is the address to start the poke stream from, buf is a pointer to
 * a buffer to read data from, and n is length of data to read.
 *
 * These functions assume a standard memory window that is 16-bits wide and
 * auto increments the address window (standard for the memwindow in the TS-75XX
 * and TS-4500 opencore).  They will set the base addresss adr in the adr_reg 
 * and keep reading data from dat_reg for n length of bytes.  
 */

void sbus_peekstream16(int adr_reg, int dat_reg, int adr, unsigned char *buf, int n) {
	assert(sbuslocked == 1);
        assert(n != 0);

	sbus_poke16(adr_reg, adr);
	
        while (n >= 2) {
                unsigned int s = sbus_peek16(dat_reg);
                *buf++ = s & 0xff;
                *buf++ = s >> 8;
                n -= 2;
        }

        if (n) *buf = sbus_peek16(dat_reg) & 0xff;
};


void sbus_pokestream16(int adr_reg, int dat_reg, int adr, unsigned char *buf, int n) {
	assert(sbuslocked == 1);
        assert(n != 0);

	sbus_poke16(adr_reg, adr);

        while (n >= 2) {
                unsigned int s;
                s = *buf++;
                s |= *buf++ << 8;
                sbus_poke16(dat_reg, s);
                n -= 2;
        }

        if (n) sbus_poke16(dat_reg, *buf);
}

/* reservemem() is meant to demand page ALL parts of the calling application
 * in to RAM.  This is to prevent an unavoidable kernel deadlock if your code
 * ventures in to un-paged areas while you hold the lock.  This is an issue
 * on the TS-75XX/TS-4500 when running from XNAND/SD card as nandctl and sdctl
 * both need to get the lock before they can get any data for the kernel.
 *
 * This will cause issues with threading, it is recommended to have a seperate
 * application the communicates with an IPC to the main threaded application.
 * The reason for this is so only that tiny application will be paged while it
 * has the lock as opposed to all threads of the threaded application.
 * 
 * This function should NOT be modified in any way!
 */

static void reservemem(void) {
	char dummy[32768];
	int i, pgsize;
	FILE *maps;

	pgsize = getpagesize();
	mlockall(MCL_CURRENT|MCL_FUTURE);
	for (i = 0; i < sizeof(dummy); i += 4096) {
		dummy[i] = 0;
	}

	maps = fopen("/proc/self/maps", "r"); 
	if (maps == NULL) {
		perror("/proc/self/maps");
		exit(1);
	}
	while (!feof(maps)) {
		unsigned int s, e, i, x;
		char m[PATH_MAX + 1];
		char perms[16];
		int r = fscanf(maps, "%x-%x %s %*x %x:%*x %*d",
		  &s, &e, perms, &x);
		if (r == EOF) break;
		assert (r == 4);

		i = 0;
		while ((r = fgetc(maps)) != '\n') {
			if (r == EOF) break;
			m[i++] = r;
		}
		m[i] = '\0';
		assert(s <= e && (s & 0xfff) == 0);
                if (perms[0] == 'r' && perms[3] == 'p' && x != 1)
                  while (s < e) {
                        volatile unsigned char *ptr = (unsigned char *)s;
                        unsigned char d;
                        d = *ptr;
                        if (perms[1] == 'w') *ptr = d;
                        s += pgsize;
                }
        }
        fclose(maps);
}

/* The following functions control the locking, unlocking and preemption
 * of the SBUS and aplications that use the SBUS.  It is recommended to only
 * acquire the SBUS lock when your application needs it.  The lock can
 * immediately be released or sbuspreempt() can be called to give up the lock
 * temporarily to any other applications if they are waiting on it.  The act
 * of locking and unlocking can be CPU intensive, if possible it is
 * recommended to use sbuspreempt() at strategic locations in the end 
 * application so the application only has the lock when it is needed and it 
 * can be given up to other applications easily.
 *
 * These functions should NOT be modified in any way!
 */

static int semid = -1;
void sbuslock(void) {
	int r;
	struct sembuf sop;
	static int inited = 0;
	if (semid == -1) {
		key_t semkey;
		reservemem();
		semkey = 0x75000000;
		semid = semget(semkey, 1, IPC_CREAT|IPC_EXCL|0777);
		if (semid != -1) {
			sop.sem_num = 0;
			sop.sem_op = 1;
			sop.sem_flg = 0;
			r = semop(semid, &sop, 1);
			assert (r != -1);
		} else semid = semget(semkey, 1, 0777);
		assert (semid != -1);
	}
	sop.sem_num = 0;
	sop.sem_op = -1;
	sop.sem_flg = SEM_UNDO;

	/*Wrapper added to retry in case of EINTR*/
	r = TEMP_FAILURE_RETRY(semop(semid, &sop, 1));

	assert (r == 0);
	if (inited == 0) {
		int i;
		int devmem;

		inited = 1;
		devmem = open("/dev/mem", O_RDWR|O_SYNC);
		assert(devmem != -1);
		cvspiregs = (unsigned int *) mmap(0, 4096,
		  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x71000000);
		cvgpioregs = (unsigned int *) mmap(0, 4096,
		  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x7c000000);

		cvspiregs[0x64/4] = 0x0; /* RX IRQ threahold 0 */
		cvspiregs[0x40/4] = 0x80000c02; /* 24-bit mode no byte swap */
		cvspiregs[0x60/4] = 0x0; /* 0 clock inter-transfer delay */
		cvspiregs[0x6c/4] = 0x0; /* disable interrupts */
		cvspiregs[0x4c/4] = 0x4; /* deassert CS# */
		for (i = 0; i < 8; i++) cvspiregs[0x58 / 4];
		last_gpio_adr = 3;
	}
	cvgpioregs[0] = (cvgpioregs[0] & ~(0x3<<15))|(last_gpio_adr<<15);
	sbuslocked = 1;
}


void sbusunlock(void) {
	struct sembuf sop = { 0, 1, SEM_UNDO};
	int r;
	if (!sbuslocked) return;
	r = semop(semid, &sop, 1);
	assert (r == 0);
	sbuslocked = 0;
}


void sbuspreempt(void) {
	int r;
	r = semctl(semid, 0, GETNCNT);
	assert (r != -1);
	if (r) {
		sbusunlock();
		sched_yield();
		sbuslock();
	}
}
