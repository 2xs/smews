#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <math.h>
#include <termios.h>

/* configuration macros */
#define DEVTUN "/dev/net/tun"

/* common globals */
#define DEV_MTU 1500
unsigned char tbuffer_shadow[DEV_MTU+4]={0x0,0x0,0x86,0xdd};
#define tbuffer (tbuffer_shadow+4)
unsigned char sbuffer_shadow[DEV_MTU+4]={0x0,0x0,0x86,0xdd};
#define sbuffer (sbuffer_shadow+4)
unsigned char *scurr = sbuffer;

/* misc. macros */
#define MAX(a,b) ((a)>(b)?(a):(b))

/* SLIP */
#define SLIP_END             0xC0    /* indicates end of packet */
#define SLIP_ESC             0xDB    /* indicates byte stuffing */
#define SLIP_ESC_END         0xDC    /* ESC ESC_END means END data byte */
#define SLIP_ESC_ESC         0xDD    /* ESC ESC_ESC means ESC data byte */

static int ret;
static int tun_fd;
static int serial_fd;
static int forward_to_tun = 0;

static void check(int test,const char *message) {
	if(!test) {
		perror(message);
		exit(1);
	}
}

static int configure_port(int fd) {      // configure the port
        struct termios port_settings;      // structure to store the port settings in
        memset(&port_settings, 0, sizeof(struct termios));
        cfsetispeed(&port_settings, B115200);    // set baud rates
        cfsetospeed(&port_settings, B115200);

        port_settings.c_cflag |= CREAD;
        port_settings.c_cflag |= CSTOPB;
        port_settings.c_cflag |= CLOCAL;
        port_settings.c_cflag |= CS8;

        return tcsetattr(fd, TCSANOW, &port_settings);    // apply the settings to the port
}

static void writeserial(unsigned char c) {
        ret = write(serial_fd, &c, 1);
        check(ret != -1,"write() to serial error");
}

static void read_from_tun() {
        int i;
        int nRead;
        /* something has been received on tun_fd, we forward it to socket_fd */
        nRead = read(tun_fd, (void *)tbuffer_shadow, DEV_MTU);
        check(nRead != -1,"read() from tun error");
        nRead -= 4;
        printf("tunToSerial %d\n",nRead);
        writeserial(SLIP_END);
        for(i=0; i<nRead; i++) {
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

static void read_from_serial() {
        int nRead;
        unsigned char tmp_buff[64];
        enum slip_state_e{slip_in,slip_escaped};
        static volatile enum slip_state_e slip_state = slip_in;
        /* something has been received on socket_fd */
        while((nRead = read(serial_fd, tmp_buff, 64)) > 0) {
	      int i;
	      for(i=0; i<nRead; i++) {
		    unsigned char c = tmp_buff[i];
		    unsigned char isChar = 1;
		    /* SLIP management */
		    switch(c) {
			  case SLIP_END:
				if(scurr != sbuffer) {
				        forward_to_tun = 1;
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
		    if(isChar && scurr < sbuffer + DEV_MTU) {
			  *scurr++ =  c;
		    }
		    if(forward_to_tun == 1) {
			  printf("serialToTun %d\n",scurr-sbuffer);
			  ret = write(tun_fd, (void *)sbuffer_shadow, scurr-sbuffer + 4);
			  check(ret != -1,"write() to tun error");
			  forward_to_tun = 0;
			  scurr = sbuffer;
		    }
	      }
        }
        check(nRead != -1,"read() from serial error");
}

int main(int argc, char **argv) {
	/* misc. locals... */
	int nfds;
	fd_set fdset;
	char tmpstr[64];
	
	if(argc != 2) {
	        fprintf(stderr,"Usage: %s <IPv6/mask>\n",argv[0]);
	        return;
	}
	
	/* open the serial port */
	serial_fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY);
	check(serial_fd != -1,"serial open() error");
	ret = configure_port(serial_fd);
	check(ret != -1,"serial configure_port() error");

	/* TUN interface creation */
	struct ifreq ifr;
	tun_fd = open(DEVTUN, O_RDWR);
	check(tun_fd != -1,"tun open() error");
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TUN;
	ret = ioctl(tun_fd, TUNSETIFF, (void *) &ifr);
	check(ret >= 0,"tun ioctl error");
	/* TUN attachement to an IP address */
	snprintf((char*)tmpstr, 64, "ifconfig %s inet6 add %s",ifr.ifr_name,argv[1]);
	ret = system((char*)tmpstr);
	check(ret != -1,"system() ifocnfig add error");
	snprintf((char*)tmpstr, 64, "ifconfig %s up",ifr.ifr_name);
	ret = system((char*)tmpstr);
	check(ret != -1,"system() ifconfig up error");
	
	/* main loop */
	nfds = MAX(tun_fd, serial_fd) + 1;
	FD_ZERO(&fdset);
	printf("Started\n");
	while(1) {
		FD_SET(tun_fd, &fdset);
		FD_SET(serial_fd, &fdset);
		
		ret = select(nfds, &fdset, NULL, NULL, NULL);
		check(ret != -1,"select() error");
		
		/* wait for something to read on tun_fd or socket_fd */
		if(ret) {
			if(FD_ISSET(tun_fd, &fdset)) {
			        read_from_tun();
			}
 			if(FD_ISSET(serial_fd, &fdset)) {
			        read_from_serial();
 			}
		}
	}
	return 0;
}
