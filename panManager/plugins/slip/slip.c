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
#include <fcntl.h>
#include <string.h>
#include <termios.h>

#include "panManager.h"

/* SLIP special bytes */
#define SLIP_END             0xC0    /* indicates end of packet */
#define SLIP_ESC             0xDB    /* indicates byte stuffing */
#define SLIP_ESC_END         0xDC    /* ESC ESC_END means END data byte */
#define SLIP_ESC_ESC         0xDD    /* ESC ESC_ESC means ESC data byte */

static int ret;
static int slip_fd;
static unsigned char *tbuffer;
static unsigned char *dbuffer;
static unsigned dev_mtu;

#define DEFAULT_SERIAL_DEV "/dev/ttyS0"

/* writes one byte to the serial line */
static void writeserial(unsigned char c) {
        ret = write(slip_fd, &c, 1);
        check(ret != -1,"write() to serial error");
}

/* callback: forward from tun to serial line using slip */
static void forward_to_slip(int size) {
        int i;
        message("Packet to slip: %d bytes\n", size);
        writeserial(SLIP_END);
        for(i=0; i<size; i++) {
	      if(tbuffer[i]==SLIP_END){
		    writeserial(SLIP_ESC);
		    writeserial(SLIP_ESC_END);
	      } else if(tbuffer[i]==SLIP_ESC){
		    writeserial(SLIP_ESC);
		    writeserial(SLIP_ESC_ESC);
	      } else {
		    writeserial(tbuffer[i]);
	      }
        }
        writeserial(SLIP_END);
}

/* callback: forward from serial line (slip) to tun */
static void read_from_slip() {
        int nRead;
        unsigned char tmp_buff[64];
        enum slip_state_e{slip_in,slip_escaped};
        /* current slip state */
        static volatile enum slip_state_e slip_state = slip_in;
        /* current position in the output buffer (inside dbuffer) */
        static unsigned char *scurr = NULL;
        
        if(scurr == NULL) {
	      scurr = dbuffer;
        }
        
        /* something has been received on socket_fd */
        while((nRead = read(slip_fd, tmp_buff, 64)) > 0) {
	      int i;
	      int ended = 0;
	      /* process the current input */
	      for(i=0; i<nRead; i++) {
		    unsigned char c = tmp_buff[i];
		    unsigned char isChar = 1;
		    /* SLIP management */
		    switch(c) {
			  case SLIP_END:
				if(scurr != dbuffer) {
				        ended = 1;
				}
				isChar = 0;
				break;
			  case SLIP_ESC:
				slip_state = slip_escaped;
				isChar = 0;
				break;
			  case SLIP_ESC_END:
				if(slip_state == slip_escaped) {
				        slip_state = slip_in;
				        c = SLIP_END;
				}
				break;
			  case SLIP_ESC_ESC:
				if(slip_state == slip_escaped) {
				        slip_state = slip_in;
				        c = SLIP_ESC;
				}
				break;
			  default:
				break;
		    }
		    /* write the new byte in the buffer is needed (and if possible) */
		    if(isChar && scurr < dbuffer + dev_mtu) {
			  *scurr++ =  c;
		    }
		    /* if the packet is ended, forward it to the tun interface */
		    if(ended == 1) {
			  message("Packet from slip: %d bytes\n", scurr-dbuffer);
			  forward_to_tun(scurr-dbuffer);
			  /* reinitializations for a new packet */
			  ended = 0;
			  scurr = dbuffer;
		    }
	      }
        }
        check(nRead != -1,"read() from serial error");
}

/* callback: initializes the serial line */
static int init_slip(unsigned char *tbuff, unsigned char *dbuff, unsigned dmtu, char *argument) {
        char *serial_dev = argument ? argument : DEFAULT_SERIAL_DEV;
        struct termios port_settings;
        
        message("Initialization of the %s serial device\n", serial_dev);
        
        /* open the serial port */
        slip_fd = open(serial_dev, O_RDWR | O_NOCTTY);
        check(slip_fd != -1,"serial open() error");

        /* configure the port */
        memset(&port_settings, 0, sizeof(struct termios));
        ret = cfsetispeed(&port_settings, B115200);
        check(slip_fd != -1,"serial cfsetispeed() error");
        
        ret = cfsetospeed(&port_settings, B115200);
        check(slip_fd != -1,"serial cfsetospeed() error");

        port_settings.c_cflag |= CREAD;
        //port_settings.c_cflag |= CSTOPB;
        port_settings.c_cflag |= CLOCAL;
        port_settings.c_cflag |= CS8;
        ret = tcsetattr(slip_fd, TCSANOW, &port_settings);
        check(ret != -1,"serial configure_port() error");
                
        /* set global variables */
        tbuffer = tbuff;
        dbuffer = dbuff;
        dev_mtu = dmtu;
        
        return slip_fd;
}

/* plugin callbacks declaration */
struct dev_handlers_s plugin_handlers = {
        "set serial device",
        init_slip,
        read_from_slip,
        forward_to_slip
};
