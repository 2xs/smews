#include <stdio.h>
#include <stdlib.h>
#include "scard.h"

utun tunnel;
SCARDCONTEXT hContext;
SCARDHANDLE hCard;

int main(int argc, char *argv[]) {
	init_signals();

	if(init_terminal(&hContext,&hCard))
		return -1;
    else {
		if(open_tunnel(&tunnel) !=-1) {
			int output_len;
			raw_packet packet;
			short sw = 0x9000 ;

			printf("SCARD host adapter is now ready to accept connections\n");

			/* main loop */
			while((output_len = utun_read(&tunnel, packet.raw, MTU)) >= 0) {
				LONG card_status = 0;
				int select_ret;
				BYTE *packet_ptr;

				if(output_len == 0)
					continue;

				select_ret = utun_select(&tunnel);

				/* send the last tun input to the card */
				printf("send %d bytes\n",output_len);
				packet_ptr = packet.raw;
				do {
					unsigned char curr_len = output_len > 255 ? 255 : output_len;
					sw = send_apdu(hContext,hCard,packet_ptr,curr_len,&card_status,select_ret);
					
					if(SW1(sw) < 0x90) {
						printf("Error: %02x%02x\n",SW1(sw),SW2(sw));
						break;
					}
					
					packet_ptr += curr_len;
					output_len -= curr_len;
				} while(output_len > 0);

				/* if there is something else to send, do it now */
				if(select_ret>0) {
					memset(&packet,0,sizeof(raw_packet)); 
					continue;
				}

				/* receive data from the card (and forward it to the tun) if needed */
				while(SW2(sw)) {
					sw = recv_apdu(hContext,hCard,&tunnel,sw,&card_status);
					if(SW1(sw) < 0x90 ) {
						printf("Error: %02x%02x\n",SW1(sw),SW2(sw));
						break;
					}
				}

				if(card_status) {
					check_terminal_status(hContext,hCard);
				}
				memset(&packet,0,sizeof(raw_packet)); 
			}
		}
	}

	close_tunnel(&tunnel);
	release_terminal(&hContext,&hCard);

	return 0;
} 
