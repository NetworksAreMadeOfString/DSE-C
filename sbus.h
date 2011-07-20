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
 * 12/16/2010 - KB
 *   Included win* functions for memwindow with TS-4500
 *   Included sbus_*stream16 functions to read a buffer of specified lenght
 *      from the SBUS mem window
 *   
 */

#ifndef SBUS_H
#define SBUS_H
void sbuslock(void);
void sbusunlock(void);
void sbuspreempt(void);

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
#endif
