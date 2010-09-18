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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pcsclite.h>
#include <winscard.h>
#include <string.h>
#include "panManager.h"

/* status bytes wrappers */
#define SW1(_sw) ((_sw) & 0xFF)
#define SW2(_sw) (((_sw)>>8) & 0xFF)
/* size of APDU headers */
#define APDU_HEADER_SIZE 5

static int ret;
static SCARDCONTEXT sc_context;
static SCARDHANDLE sc_handle;
static unsigned char *tbuffer;
static unsigned char *dbuffer;
static unsigned dev_mtu;

/* process a check including the pcsc specific error */
static void pcsccheck(int val, const char *str) {
        check(val == 0, "%s: %s", str, pcsc_stringify_error(val));
}

/* send one IP packet using APDUs */
static short apduip_send_packet(unsigned char *buffer, int size) {
        short sw;
        unsigned long in_size = 255 + 3;
        unsigned long out_size = 255 + 3;
        unsigned char in_buffer[in_size];
        unsigned char out_buffer[out_size];
        unsigned char *curr_output_ptr = buffer;

        /* set the APDU header */
        out_buffer[0] = 0x80;
        out_buffer[1] = 0x01;
        out_buffer[2] = 0x00;
        out_buffer[3] = 0;
	
        /* loop to send the packet via multiple APDUs */
        do {
	      /* size of the current message */
	      unsigned char curr_len = size > 255 ? 255 : size;
	      out_size = curr_len + APDU_HEADER_SIZE;
	      
	      /* set the buffer to be sent */
	      out_buffer[4] = curr_len;
	      memcpy(out_buffer + APDU_HEADER_SIZE, curr_output_ptr, curr_len);

	      /* process APDU */
	      ret = SCardTransmit(sc_handle, SCARD_PCI_T0, out_buffer, out_size, NULL, in_buffer, &in_size);
	      pcsccheck(ret, "SCardTransmit() error (descending IP)");

	      /* check status word */
	      sw = *((short*)(in_buffer + in_size - 2));
	      check(SW1(sw) >= 0x90, "APDU error code (descending IP): 0x%02x%02x\n", SW1(sw), SW2(sw));

	      /* update pointers and remainig size */
	      curr_output_ptr += curr_len;
	      size -= curr_len;

        } while(size > 0);
        
        return sw;
}

/* receive one IP packet using APDUs */
short apduip_recv_packet(unsigned short sw, unsigned char *buffer) {
        unsigned long out_size = APDU_HEADER_SIZE;
        unsigned long in_size = 255 + 3;
        unsigned char out_buffer[APDU_HEADER_SIZE];
        unsigned char *curr_input_ptr = buffer;
        int packet_length = -1;

        /* set the APDU header */
        out_buffer[0]= 0x80;
        out_buffer[1]= 0x02;
        out_buffer[2]= 0x00;
        out_buffer[3]= 0x00;
        out_buffer[4]= SW2(sw);

        /* loop to receive the packet from multiple APDUs */
        do {
	      /* process APDU */
	      out_buffer[4]= SW2(sw);
	      ret = SCardTransmit(sc_handle, SCARD_PCI_T0, out_buffer, out_size, NULL, curr_input_ptr, &in_size);
	      pcsccheck(ret, "SCardTransmit() error (ascending IP)");

	      /* check status word */
	      sw = *((short*)(curr_input_ptr + in_size - 2));
	      check(SW1(sw) >= 0x90, "APDU error code (ascending IP): 0x%02x%02x\n", SW1(sw), SW2(sw));

	      /* set the initial IP packet length (contained in the first APDU response) */
	      if(packet_length == -1) {
		    packet_length = (curr_input_ptr[2] << 8) + curr_input_ptr[3];
		    check(packet_length <= dev_mtu, "IP packet bigger than MTU (%d > %d)\n", packet_length, dev_mtu);
	      }

	      /* update pointers and remainig size */
	      packet_length -= in_size - 2;
	      curr_input_ptr += in_size - 2;
        } while(packet_length > 0);

        /* forward the packet to the tu ninterface */
        forward_to_tun(curr_input_ptr - buffer);

        return sw;
}

/* callback: called when a packet has been received from the tun interface */
static void forward_to_apduip(int size) {
        unsigned char *curr_data = tbuffer;
        short sw;
        
        /* send the current tun input to the card */
        sw = apduip_send_packet(curr_data, size);
        
        /* receive all ascending data from the card and forward it to the tun if needed */
        while(SW2(sw)) {
	      /* the status word allows to know if the card has other packets to send */
	      sw = apduip_recv_packet(sw, dbuffer);
        }
}

/* callback: initializes the card reader */
static int init_apduip(unsigned char *tbuff, unsigned char *dbuff, unsigned dmtu, char *argument) {
        char *cr_names;
        unsigned long cr_names_len, protocol;

        /* initialization and reader detection*/
        ret = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &sc_context);
        pcsccheck(ret, "SCardEstablishContext() error");
        
        ret = SCardListReaders(sc_context, NULL, NULL, &cr_names_len);
        pcsccheck(ret, "ListReaders() error");
        
        cr_names = malloc(cr_names_len);
        ret = SCardListReaders(sc_context, NULL, cr_names, &cr_names_len);
        pcsccheck(ret, "ListReaders() error");

        message("Please insert a card in the reader: %s\n",cr_names);

        /* loop until a card has been inserted */
        do {
	      usleep(100);
	      ret = SCardConnect(sc_context, cr_names, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0, &sc_handle, &protocol);
        } while (ret == SCARD_E_NO_SMARTCARD || ret == SCARD_W_REMOVED_CARD);
        pcsccheck(ret, "SCardConnect() error");

        /* set global variables */
        tbuffer = tbuff;
        dbuffer = dbuff;
        dev_mtu = dmtu;
        
        return 0;
}

/* plugin callbacks declaration */
struct dev_handlers_s plugin_handlers = {
        "",
        init_apduip,
        NULL, /* this callback is unused here: with APDUs, no data can come from the card without having been requested by the reader */
        forward_to_apduip
};
