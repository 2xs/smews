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

#include "types.h"
#include "target.h"
#include "connections.h"


#include <stdio.h>
#include <arpa/inet.h>
#include <sys/time.h>

unsigned char in_buffer[INBUF_SIZE];
unsigned char out_buffer[OUTBUF_SIZE];

int in_curr;
int out_curr;
int in_nbytes;
struct timeval tv;
fd_set fdset;
int smewsId;
int socket_fd;
struct sockaddr_in sockaddr;
static int ret;

/*-----------------------------------------------------------------------------------*/
static void check(int test,const char *message) {
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
	/* get ID from environ */
	if(getenv("SMEWS_ID"))
		smewsId = atoi(getenv("SMEWS_ID"));
	else
		smewsId = 0;

	/* set IP address */
	local_ip_addr[3] = 192;
	local_ip_addr[2] = 168;
	local_ip_addr[1] = 1 + (smewsId >> 8);
	local_ip_addr[0] = smewsId % 0xff;
	
	/* create socket */
	socket_fd = socket(AF_INET,SOCK_DGRAM,0);
	check(socket_fd != -1,"socket() error");
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(1024 + 1 + smewsId);
	sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(&sockaddr.sin_zero,0,8);
	ret = bind(socket_fd,(struct sockaddr*)&sockaddr,sizeof(sockaddr));
	check(ret != -1,"bind() error");
	
	/* set sockaddr for output */
	sockaddr.sin_port = htons(1024);

	/* initialize fdset */
	FD_ZERO(&fdset);

	/* do a first void output */
	dev_output_done();
}

/*-----------------------------------------------------------------------------------*/
void wait_input() {
	tv.tv_sec = 0;
	tv.tv_usec = 50000;
	FD_SET(socket_fd, &fdset);
	ret = select(socket_fd + 1, &fdset, NULL, NULL, &tv);
	check(ret != -1,"select() error");
}


/*-----------------------------------------------------------------------------------*/
unsigned char dev_data_to_read() {
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_SET(socket_fd, &fdset);
	ret = select(socket_fd + 1, &fdset, NULL, NULL, &tv);
	check(ret != -1,"select() error");
	return ret;
}

/*-----------------------------------------------------------------------------------*/
int16_t dev_get(void) {
	unsigned char c;
	if(in_curr >= in_nbytes - 1) {
		int ret;
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		FD_SET(socket_fd, &fdset);
		ret = select(socket_fd + 1, &fdset, NULL, NULL, &tv);
		check(ret != -1,"select() error");
		if(ret == 0) {
			return -1;
		} else {
			ret = recv(socket_fd,in_buffer,INBUF_SIZE,0);
			if(ret == -1) {
				return -1;
			} else {
				in_nbytes = ret;
				in_curr = -1;
				c = in_buffer[++in_curr];
			}
		}
	} else {
		c = in_buffer[++in_curr];
	}
	return c;
}

/*-----------------------------------------------------------------------------------*/
void dev_prepare_output() {
	out_curr=0;
}

/*-----------------------------------------------------------------------------------*/
void dev_output_done(void) {
	ret = sendto(socket_fd,out_buffer,out_curr,0,(struct sockaddr*)&sockaddr,sizeof(sockaddr));
	check(ret != -1,"sendto() error");
}
