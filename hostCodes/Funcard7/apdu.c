#include <time.h>
#include "scard.h"
#define APDU_HEADER_SIZE 5

short send_apdu(SCARDCONTEXT hContext,SCARDHANDLE hCard,char *paquet,int len,LONG *rv,int P2) {
	short sw;
	BYTE pbRecvBuffer[MTU+1+2];
	BYTE *pbSendBuffer;
	DWORD dwSendLength = len + APDU_HEADER_SIZE;
	DWORD dwRecvLength = sizeof(pbRecvBuffer);

	pbSendBuffer = malloc(dwSendLength);
	pbSendBuffer[0] = 0x80;
	pbSendBuffer[1] = 0x01;
	pbSendBuffer[2] = 0x00;
	pbSendBuffer[3] = P2;
	pbSendBuffer[4] = len;
	memcpy(pbSendBuffer+APDU_HEADER_SIZE,paquet,len);

	*rv = SCardTransmit(hCard, 
		SCARD_PCI_T0, 
		pbSendBuffer,
		dwSendLength,
		NULL,//&pioRecvPci, 
		pbRecvBuffer, 
		&dwRecvLength);

	sw = *((short*)(pbRecvBuffer+dwRecvLength-2));
	printf("   %d bytes. %02x%02x. %s \n", len, SW1(sw),SW2(sw), pcsc_stringify_error(*rv));

	if(*rv) {
		if(check_terminal_status(hContext,hCard)==-1)
			return -1;
		else
			return 0x90;
	}

	return sw;
}


short recv_apdu(SCARDCONTEXT hContext,SCARDHANDLE hCard,utun *t,unsigned short sw,LONG *rv) {
	SCARD_IO_REQUEST pioRecvPci;
	BYTE pbRecvBuffer[MTU+1+2];
	BYTE pbSendBuffer[APDU_HEADER_SIZE];
	BYTE *recvBuffPtr = pbRecvBuffer;
	int packet_length = -1;

	DWORD dwSendLength = APDU_HEADER_SIZE;
	DWORD dwRecvLength = sizeof(pbRecvBuffer);

	pbSendBuffer[0]= 0x80;
	pbSendBuffer[1]= 0x02;
	pbSendBuffer[2]= 0x00;
	pbSendBuffer[3]= 0x00;
	pbSendBuffer[4]= SW2(sw);

	do {
		pbSendBuffer[4]= SW2(sw);
		*rv = SCardTransmit(hCard, SCARD_PCI_T0,
			pbSendBuffer,
			dwSendLength,
			#ifdef WIN32
			NULL,
			#else
			&pioRecvPci, 
			#endif
			recvBuffPtr, 
			&dwRecvLength);

		sw = *((short*)(recvBuffPtr+dwRecvLength-2));

		if(packet_length == -1) {
			packet_length = (recvBuffPtr[2] << 8) + recvBuffPtr[3];
			printf("recv %d bytes\n",packet_length);
		}

		printf("   %d bytes. %02x%02x. %s\n",dwRecvLength-2,SW1(sw),SW2(sw),pcsc_stringify_error(*rv));

		if(*rv) {
			if(check_terminal_status(hContext,hCard)==-1)
				return -1;
			else
				return 0x90;
		}

		packet_length -= dwRecvLength -2;

		recvBuffPtr += dwRecvLength-2;
	} while(packet_length > 0);

	utun_write(t, pbRecvBuffer,recvBuffPtr-pbRecvBuffer);

	return sw;
}
