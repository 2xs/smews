/*
* Copyright or © or Copr. 2008, Simon Duquennoy
* 
* Author e-mail: simon.duquennoy@lifl.fr
* 
* This software is a computer program whose purpose is to design an
* efficient Web server for very-constrained embedded system.
* 
* This software is governed by the CeCILL license under French law and
* abiding by the rules of distribution of free software.  You can  use, 
* modify and/ or redistribute the software under the terms of the CeCILL
* license as circulated by CEA, CNRS and INRIA at the following URL
* "http://www.cecill.info". 
* 
* As a counterpart to the access to the source code and  rights to copy,
* modify and redistribute granted by the license, users are provided only
* with a limited warranty  and the software's author,  the holder of the
* economic rights,  and the successive licensors  have only  limited
* liability. 
* 
* In this respect, the user's attention is drawn to the risks associated
* with loading,  using,  modifying and/or developing or reproducing the
* software by the user in light of its specific status of free software,
* that may mean  that it is complicated to manipulate,  and  that  also
* therefore means  that it is reserved for developers  and  experienced
* professionals having in-depth computer knowledge. Users are therefore
* encouraged to load and test the software's suitability as regards their
* requirements in conditions enabling the security of their systems and/or 
* data to be ensured and,  more generally, to use and operate it in the 
* same conditions as regards security. 
* 
* The fact that you are presently reading this means that you have had
* knowledge of the CeCILL license and that you accept its terms.
*/


#include "target.h"
#include "slip_dev.h"


struct TIMERCHN {
	unsigned short counter;
	unsigned short control;
};

#define REG_IE (*(volatile unsigned short *) 0x04000200)
#define IT_ENABLE 0x0001
#define IT_SERIAL 0x0080
#define IT_TIMER0 0x0008
#define IT_KEY 0x1000
#define TIMER ((volatile struct TIMERCHN *) 0x04000100)
#define TIMER_START 0x80
#define TIMER_IRQ 0x40
#define TIMER_COUNTUP 0x04
#define TIMER_SYSCLOCK 0x00
#define TIMER_SYSCLOCK64 0x01
#define TIMER_SYSCLOCK256 0x02
#define TIMER_SYSCLOCK1024 0x03
#define REG_IME	(*(volatile unsigned short *) 0x04000208)
#define REG_IE (*(volatile unsigned short *) 0x04000200)
#define REG_IF (*(volatile unsigned short *) 0x04000202)


/* KEYPAD INPUT AND CONTROL REGISTERS */
#define REG_KEY		(*(volatile unsigned short *) 0x4000130)
#define REG_P1CNT	(*(volatile unsigned short *) 0x4000132)

/* KEY PAD DEFINES */
#define KEY_A		0x0001
#define KEY_B		0x0002
#define KEY_SELECT	0x0004
#define KEY_START	0x0008
#define KEY_RIGHT	0x0010
#define KEY_LEFT	0x0020
#define KEY_UP		0x0040
#define KEY_DOWN	0x0080
#define KEY_R		0x0100
#define KEY_L		0x0200

volatile uint32_t global_timer;

static void ret(void){}
static void _int_timer0();

const void * const IntrTable[14] = {
	ret,
	ret,
	ret,
	_int_timer0,
	ret,
	ret,
	ret,
	_int_serial_com,
	ret,
	ret,
	ret,
	ret,
	ret,
	ret
};

static void _int_timer0() {
	// Disable this interrupt
	REG_IE &= ~IT_TIMER0;
	global_timer++;
	TIMER[0].counter = 65535 - 16780;
	TIMER[0].control = TIMER_START | TIMER_IRQ | TIMER_SYSCLOCK;
	REG_IE |= IT_TIMER0;
}

void hardware_timer_init(void) {
	REG_IME	= IT_ENABLE;
	REG_IE	= IT_TIMER0 | IT_SERIAL;
	TIMER[0].counter = 65535 - 16780;
	TIMER[0].control = TIMER_START | TIMER_IRQ | TIMER_SYSCLOCK;
	global_timer = 0;
}

void hardware_init() {
	hardware_timer_init();
	dev_init();
}
