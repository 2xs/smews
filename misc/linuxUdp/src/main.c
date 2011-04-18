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

/* configuration macros */
#define DEVTUN "/dev/net/tun"

/* common globals */
#define BUFSIZE 2048
unsigned char buffer_shadow[BUFSIZE+4]={0x0,0x0,0x08,0x0};
#define buffer (buffer_shadow+4)

/* misc. macros */
#define MAX(a,b) ((a)>(b)?(a):(b))

void check(int test,const char *message) {
	if(!test) {
		perror(message);
		exit(1);
	}
}

int main(int argc, char **argv) {

	/* misc. locals... */
	int i;
	int j;
	int ret;
	int nRead;
	int nfds;
	int tun_fd;
	int socket_fd;
	fd_set fdset;
	char tmpstr[64];
	char *cptr;
	struct sockaddr_in sockaddr;

	/* host socket creation */
	socket_fd = socket(AF_INET,SOCK_DGRAM,0);
	check(socket_fd != -1,"socket() error");
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(1024);
	sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(&sockaddr.sin_zero,0,8);
	ret=bind(socket_fd,(struct sockaddr*)&sockaddr,sizeof(sockaddr));
	check(ret != -1,"bind() error");

	/* TUN interface creation */
	struct ifreq ifr;
	tun_fd = open(DEVTUN, O_RDWR);
	check(tun_fd != -1,"tun open() error");
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TUN;
	ret = ioctl(tun_fd, TUNSETIFF, (void *) &ifr);
	check(ret >= 0,"tun ioctl error");
	/* TUN attachement to an IP address */
	snprintf((char*)tmpstr, BUFSIZE, "ifconfig %s 192.168.0.0 pointopoint 192.168.1.0 mtu 1500",ifr.ifr_name);
	ret = system((char*)tmpstr);
	check(ret != -1,"system() error when setting ifconfig");
	
	/* main loop */
	nfds = MAX(tun_fd,socket_fd)+1;
	FD_ZERO(&fdset);
	while(1) {
		FD_SET(tun_fd, &fdset);
		FD_SET(socket_fd, &fdset);
		
		ret = select(nfds, &fdset, NULL, NULL, NULL);
		check(ret != -1,"select() error");
		
		/* wait for something to read on tun_fd or socket_fd */
		if(ret) {
			if(FD_ISSET(tun_fd, &fdset)) {
				/* something has been received on tun_fd, we forward it to socket_fd */
				nRead = read(tun_fd, (void *)buffer, BUFSIZE);
				check(nRead != -1,"read() error");
				sockaddr.sin_port = htons(1025);
				ret = sendto(socket_fd,buffer,nRead,0,(struct sockaddr*)&sockaddr,sizeof(sockaddr));
				check(ret != -1,"sendto() error");
			}
			if(FD_ISSET(socket_fd, &fdset)) {
				/* something has been received on socket_fd */
				int addrLen;
				int emitterN;
				nRead = recvfrom(socket_fd,buffer,BUFSIZE,0,(struct sockaddr*)&sockaddr,&addrLen);
				check(nRead != -1,"recvfrom() error");
				emitterN = ntohs(sockaddr.sin_port) - (1025);
				if(emitterN == 0) {
					/* if the emitter is the Smews instance */
					ret = write(tun_fd, buffer_shadow, nRead+4);
					check(ret != -1,"write() error");
				}
			}
		}
	}
	return 0;
}
