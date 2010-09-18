
#include "tls/target.h"


struct sockaddr_in dest_sockaddr;  /* socket info about the machine connecting to us */
struct sockaddr_in serv_sockaddr;  /* socket info about our server */


unsigned char in_buffer[INBUF_SIZE];
unsigned char out_buffer[OUTBUF_SIZE];


int tun_fd; /* listening socket */
int consocket; /* peer socket */

int in_curr;
int out_curr;
int in_nbytes;

struct timeval tv;
fd_set fdset;

static int ret;

unsigned int socksize = sizeof(struct sockaddr_in);

/*-----------------------------------------------------------------------------------*/
void check(int test,const char *message) {
	if(!test) {
		perror(message);
		exit(1);
	}
}

/*-----------------------------------------------------------------------------------*/
unsigned int get_time(void) {
	struct timeval stime;
	ret = gettimeofday(&stime, (struct timezone*)0);
	check(ret != -1,"gettimeofday() error");
	return stime.tv_sec * 1000 + stime.tv_usec / 1000;
}

/*-----------------------------------------------------------------------------------*/
void connect_socket(){

	consocket = accept(tun_fd, (struct sockaddr *)&dest_sockaddr, &socksize);

}
/*-----------------------------------------------------------------------------------*/

void dev_init(void) {

	int yes = 1;
	int ret;

	int randPort = 1989;

	tun_fd = socket(PF_INET, SOCK_STREAM, 0);

	/* to permit reusing the same address if socket timeout did not expire */
	if(setsockopt(tun_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	memset(&dest_sockaddr, 0, sizeof(dest_sockaddr));

	serv_sockaddr.sin_family = AF_INET;
	serv_sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_sockaddr.sin_port = htons(randPort);

	bzero(serv_sockaddr.sin_zero,8);


	/* bind serv information to mysocket */
	ret = bind(tun_fd, (struct sockaddr *)&serv_sockaddr, socksize);
	check(ret != -1,"Error binding to socket");


	if (listen(tun_fd, 1) == -1) {
		perror("listen");
		exit(1);
	}


	in_curr = 0;
	in_nbytes = 0;
	out_curr = 0;

	FD_ZERO(&fdset);

	printf("Waiting for connections on port %d\n",randPort);

	int flags = fcntl(0,F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(consocket,F_SETFL,flags);
}

/*-----------------------------------------------------------------------------------*/
int16_t dev_get() {

	unsigned char c;
	if(in_curr > in_nbytes - 1) {
		int ret;
		ret = recv(consocket, (void *)in_buffer, INBUF_SIZE, 0);
		if(ret == -1) {
			perror("Error reading from socket");
			return -1;
		} else {
			in_nbytes = ret;
			in_curr = 0;
			c = in_buffer[in_curr++];
		}
		//}
	} else {
		c = in_buffer[in_curr++];
	}

	return c;

}

/*-----------------------------------------------------------------------------------*/
void wait_input() {

	tv.tv_sec = 0;
	tv.tv_usec = 50000;
	FD_SET(tun_fd, &fdset);
	//FD_SET(consocket, &fdset);

	ret = select(tun_fd + 1, &fdset, NULL, NULL, &tv);
	check(ret != -1,"select() error");

}

/*-----------------------------------------------------------------------------------*/
unsigned char dev_data_to_read() {

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_SET(tun_fd, &fdset);
	//FD_SET(consocket, &fdset);
	ret = select(tun_fd + 1, &fdset, NULL, NULL, &tv);
	check(ret != -1,"select() error");
	return ret;

}


unsigned char net_receive() {

	tv.tv_sec = 0;
	tv.tv_usec = 50000;
	FD_SET(consocket, &fdset);
	ret = select(consocket + 1, &fdset, NULL, NULL, &tv);
	check(ret != -1,"select() error");
	return ret;


}

int16_t user_input(uint8_t *buf){

	FD_SET(0, &fdset);
	ret = select(0 + 1, &fdset, NULL, NULL, &tv);
	if(ret == 0) return 0;
	int x = read(0,buf,128);
	check(x != -1,"stdin err");
	return x;

}

/*-----------------------------------------------------------------------------------*/
void dev_prepare_output() {

	out_curr = 0;
}

/*-----------------------------------------------------------------------------------*/
void dev_output_done(void) {
	ret = write(consocket, out_buffer, out_curr);
	check(ret != -1,"write() error");
}
