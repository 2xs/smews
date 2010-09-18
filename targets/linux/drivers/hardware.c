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
#include "connections.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/time.h>
#include <fcntl.h>

#define DEVTUN "/dev/net/tun"

unsigned char in_buffer[INBUF_SIZE];
#ifdef IPV6
unsigned char out_buffer[OUTBUF_SIZE]={0x0,0x0,0x86,0xdd};
#else
unsigned char out_buffer[OUTBUF_SIZE]={0x0,0x0,0x08,0x0};
#endif

int tun_fd;
int in_curr;
int out_curr;
int in_nbytes;
struct timeval tv;
fd_set fdset;
static int ret;

/*-----------------------------------------------------------------------------------*/
void check(int test,const char *message) {
	if(!test) {
		perror(message);
		exit(1);
	}
}

/*-----------------------------------------------------------------------------------*/
uint32_t get_time(void) {
	struct timeval stime;
	ret = gettimeofday(&stime, (struct timezone*)0);
	check(ret != -1,"gettimeofday() error");
	return stime.tv_sec * 1000 + stime.tv_usec / 1000;
}

/*-----------------------------------------------------------------------------------*/
void dev_init(void) {
	struct ifreq ifr;

#ifdef IPV6
	int lastBit;

	if ((local_ip_addr[0] % 2))
		 lastBit = local_ip_addr[0]-1;
	else
		 lastBit = local_ip_addr[0]+1;

#endif

	tun_fd = open(DEVTUN, O_RDWR);
	check(tun_fd != -1,"tun open() error");
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TUN;
	ret = ioctl(tun_fd, TUNSETIFF, (void *) &ifr);
	check(ret != -1,"ioctl() error");
#ifdef IPV6
	snprintf((char*)out_buffer+4, sizeof(out_buffer)-4, "ifconfig %s inet6 add %.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x/127",ifr.ifr_name,local_ip_addr[15],local_ip_addr[14],local_ip_addr[13],local_ip_addr[12],local_ip_addr[11],local_ip_addr[10],local_ip_addr[9],local_ip_addr[8],local_ip_addr[7],local_ip_addr[6],local_ip_addr[5],local_ip_addr[4],local_ip_addr[3],local_ip_addr[2],local_ip_addr[1],lastBit);
	ret = system((char*)out_buffer+4);
	check(ret != -1,"system() error");
	snprintf((char*)out_buffer+4, sizeof(out_buffer)-4, "ifconfig %s up", ifr.ifr_name);
#else
	snprintf((char*)out_buffer+4, sizeof(out_buffer)-4, "ifconfig %s 192.168.0.1 pointopoint %d.%d.%d.%d mtu 1500",ifr.ifr_name,local_ip_addr[3],local_ip_addr[2],local_ip_addr[1],local_ip_addr[0]);
#endif
	ret = system((char*)out_buffer+4);
	check(ret != -1,"system() error");

	in_curr = 0;
	in_nbytes = 0;
	out_curr = 4;
	FD_ZERO(&fdset);
}

/*-----------------------------------------------------------------------------------*/
int16_t dev_get() {
	unsigned char c;
	if(in_curr >= in_nbytes - 1) {
		int ret;
		tv.tv_sec = 0;
		tv.tv_usec = 50000;
		FD_SET(tun_fd, &fdset);
		ret = select(tun_fd + 1, &fdset, NULL, NULL, &tv);
		check(ret != -1,"select() error");
		if(ret == 0) {
			return -1;
		} else {
			ret = read(tun_fd, (void *)in_buffer, INBUF_SIZE);
			if(ret == -1) {
				return -1;
			} else {
				in_nbytes = ret;
				in_curr = -1 + 4;
				c = in_buffer[++in_curr];
			}
		}
	} else {
		c = in_buffer[++in_curr];
	}
	return c;
}

/*-----------------------------------------------------------------------------------*/
void wait_input() {
	tv.tv_sec = 0;
	tv.tv_usec = 50000;
	FD_SET(tun_fd, &fdset);
	ret = select(tun_fd + 1, &fdset, NULL, NULL, &tv);
	check(ret != -1,"select() error");
}

/*-----------------------------------------------------------------------------------*/
unsigned char dev_data_to_read() {
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_SET(tun_fd, &fdset);
	ret = select(tun_fd + 1, &fdset, NULL, NULL, &tv);
	check(ret != -1,"select() error");
	return ret;
}

/*-----------------------------------------------------------------------------------*/
void dev_prepare_output() {
	out_curr=4;
}

/*-----------------------------------------------------------------------------------*/
void dev_output_done(void) {
	ret = write(tun_fd, out_buffer, out_curr);
	check(ret != -1,"write() error");
}
