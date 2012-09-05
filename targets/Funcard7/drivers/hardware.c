/*
* Copyright or © or Copr. 2008, Geoffroy Cogniaux, Simon Duquennoy,
* Thomas Soete
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

#include "at_config.h"
#include "types.h"
#include "target.h"

/* Structures used for efficient casts */
union word_dword_u {
	uint16_t word[2];
	uint32_t dword;
};
union byte_dword_u {
	uint8_t byte[4];
	uint32_t dword;
};
union byte_word_u {
	unsigned char byte[2];
	uint16_t word;
};

/* T0 protocol C wrappers */
//io.s
extern void sendbyteT0(unsigned char b);
extern unsigned char dropByteT0(void);
extern void sendFbyteT0(unsigned char b);
extern unsigned char recFbyteT0(void);
//eepromi2c.s
extern void xewrt(uint16_t *addr, unsigned char value);
extern uint8_t fxeread(uint8_t* addr);
extern uint8_t fxeread_next();

#define T0_sendFAck(header) sendFbyteT0((header)[1])
#define T0_sendFCAck(header) sendFbyteT0(~(header)[1])

/* APDU definitions used by smews */
#define CLA_PROP 0x80
#define INS_SEND_PACKET	 0x01
#define INS_RECV_PACKET 0x02

/* Wrappers on APDU header struct */
static unsigned char header[5];
#define H_CLA(header) ((header)[0])
#define H_INS(header) ((header)[1])
#define H_P1(header) ((header)[2])
#define H_P2(header) ((header)[3])
#define H_LE(header) ((header)[4])

uint16_t last_addr = -1;

static uint8_t max_read;
static uint8_t curr_read;

static uint8_t nwrite;
static uint16_t segment_left;
#define APDU_SIZE 255

#define PTS_0 0xff
#define PTS_1 0x10
#define PTS_2 0x08
#define PTS_3 0xE7

/* unallows CONST_VAR to target the zero address */
static unsigned char null_eeprom_ptr EEMEM = 0xFF;

#define ATR_LEN_ADDR 16
static unsigned char atr[] EEMEM = {
	0x3B, //TS  - direct convention
	0xBA, //T0  -TA1/ TB1/TD1 follows,  historical bytes,
	0x08, //Fi=372 - Di = 12 (31clocks/bit) 115200 Baud
	0x00, //TB1 - no Vpp, deprecated
	0x40, //TD1 - TC2 follows
	0x20, //TC2 - WI 32 : T0 specific=>Waiting time
	0x53, // - SMEWS : (On Smart Card)
	0x4D,
	0x45,
	0x57,
	0x53,
	YEAR, //add at compilation time
	MONTH, //add at compilation time
	DAY, //add at compilation time
	CHIP, //- Chip type, @see at_config.h
	ESIZ, //- External EEPROM size, @see at_config.h
};

void T0_sendFWord(uint16_t word) {
	sendFbyteT0(word >> 8);
	sendFbyteT0(word);
}

static char check_apdu() {
	if((H_CLA(header) & 0xFC) != CLA_PROP ) {
		T0_sendFWord(SW_WRONG_CLA);
		return 0;
	}
	if(H_INS(header) > 0x02 ) {
		T0_sendFWord(SW_WRONG_INS);
		return 0;
	}
	return 1;
}

static void get_apdu() {
	do {
		H_CLA(header) = recFbyteT0();
		H_INS(header) = recFbyteT0();
		H_P1(header) = recFbyteT0();
		H_P2(header) = recFbyteT0();
		H_LE(header) = recFbyteT0();
	} while(!check_apdu());
}

void hardware_init(void) {
	uint8_t i;

	/* ports init */
	ACSR = 0x80;
	DDRA = 0xFF;
	DDRB = 0xFF;
	DDRC = 0xFF;
	DDRD = 0xFF;
	PORTA = 0xFF;
	PORTB = 0xFF;
	PORTC = 0xFF;
	PORTD = 0xFF;

	/* Send ATR */
	for( i=0; i < ATR_LEN_ADDR ; i++ ) {
		sendbyteT0(CONST_READ_UI8(atr+i));
	}

	dropByteT0();
	dropByteT0();
	dropByteT0();
	dropByteT0();
	sendbyteT0(PTS_0);
	sendbyteT0(PTS_1);
	sendbyteT0(PTS_2);
	sendbyteT0(PTS_3);

	/* initialize state, terminal is the first who talks, so we wait for it */
	get_apdu();
	curr_read = 0;
	max_read = H_LE(header);
}

void init_read() {
	T0_sendFWord(SW_OK);
	get_apdu();
	curr_read = 0;
	max_read = H_LE(header);
}

unsigned char dev_get(void) {
	if(curr_read == max_read) {
		init_read();
	}
	/* change com direction : ~ACK */
	T0_sendFCAck(header);
	curr_read++;
	/* explicit recv of data : */
	return recFbyteT0();
}

char dev_prepare_output(uint16_t len) {
	unsigned char out_size = len > 255 ? 255 : len; 
	nwrite = 0;
	T0_sendFWord(SW_OK | out_size);
	segment_left = len - out_size;
	
	/* initialize write state, terminal is the first who talks, so we wait for it */
	get_apdu();
	/* change com direction */
	T0_sendFAck(header);

	return 1;
}

unsigned char dev_data_to_read(void) {
	return max_read - curr_read > 0 || H_P2(header) > 0;
}

void dev_put(unsigned char byte) {
	sendFbyteT0(byte);
	if(++nwrite == APDU_SIZE)
		dev_prepare_output(segment_left);
}

/* extended EEPROM routines */
uint32_t eeprom_read_dword(const void *addr) {
	union byte_dword_u tmp_cast;
	tmp_cast.byte[0] = fxeread((uint8_t*)addr);
	tmp_cast.byte[1] = fxeread_next();
	tmp_cast.byte[2] = fxeread_next();
	tmp_cast.byte[3] = fxeread_next();
	return tmp_cast.dword;
}

uint16_t eeprom_read_word(const void *addr) {
	union byte_word_u tmp_cast;
	tmp_cast.byte[0]= fxeread((uint8_t*)addr);
	tmp_cast.byte[1]= fxeread_next();
	return tmp_cast.word;
}

void eeprom_write_nbytes(void *dst, const unsigned char *src, uint16_t len) {
	while( len-- ) {
		xewrt(dst++, *src++);
	}
}
