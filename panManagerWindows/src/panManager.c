/*
* Copyright or © or Copr. 2010, Thomas Soete
* 
* Author e-mail: thomas@soete.org
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

#include "wintun.h"

/* Configuration */
#define DEV_MTU 1500
#define IP_FROM "192.168.2.1"
#define IP_TO   "192.168.2.2"
#define COM_PORT "\\\\.\\COM6"

/* SLIP special bytes */
#define SLIP_END             0xC0    /* indicates end of packet */
#define SLIP_ESC             0xDB    /* indicates byte stuffing */
#define SLIP_ESC_END         0xDC    /* ESC ESC_END means END data byte */
#define SLIP_ESC_ESC         0xDD    /* ESC ESC_ESC means ESC data byte */

typedef struct {
	utun * pTun;
	HANDLE serial;
} commHandles;

/* Tun receive Thread */
DWORD WINAPI TunReceiveThread(LPVOID lpParam)
{
	commHandles * handles = (commHandles*) lpParam;
	unsigned char packet[DEV_MTU];
	unsigned char packetSlip[DEV_MTU*2];
	int iRet;

	// Create TX event
	OVERLAPPED ov;
	memset(&ov,0,sizeof(ov));
	ov.hEvent = CreateEvent(0,0,0,0);

	while(1)
	{
		unsigned int nbBytesFromTun;
		unsigned int nbBytesToSerial;
		unsigned int i;
		unsigned char * pPacket;
		DWORD nbBytesWrittenToSerial;

		// Read a packet from TUN
		nbBytesFromTun = utun_read(handles->pTun, packet, DEV_MTU);

		#ifdef _DEBUG
			printf("Received %d bytes from TUN\n", nbBytesFromTun);
		#endif

		// Format as a SLIP Packet
		pPacket = packetSlip;
		*pPacket++ = SLIP_END;
		for(i = 0; i < nbBytesFromTun; i++)
		{
			unsigned char c = packet[i];
			if(c==SLIP_END){
			*pPacket++ = SLIP_ESC;
			*pPacket++ = SLIP_ESC_END;
			} else if(c==SLIP_ESC){

			*pPacket++ = SLIP_ESC;
			*pPacket++ = SLIP_ESC_ESC;
			} else {
			*pPacket++ = c;
			}

		}
		*pPacket++ = SLIP_END;

		// Write packet to serial
		nbBytesToSerial = pPacket-packetSlip;
		iRet = WriteFile(handles->serial, packetSlip, nbBytesToSerial, &nbBytesWrittenToSerial, &ov);
		if ( iRet == 0 )
		{
			WaitForSingleObject(ov.hEvent ,INFINITE);
		}

		#ifdef _DEBUG
				printf("Written %d bytes to serial\n", nbBytesToSerial);
		#endif
	}
}

/* Serial receive Thread */
DWORD WINAPI SerialReceiveThread(LPVOID lpParam)
{
    enum slip_state_e {slip_in,slip_escaped};

	commHandles * handles = (commHandles*) lpParam;
	unsigned char packet[DEV_MTU];
	unsigned int packetLength = 0;
	unsigned int ended = 0;
    volatile enum slip_state_e slip_state = slip_in;

	// Create RX Event
	DWORD dwEventMask = 0;
	OVERLAPPED ov;
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = CreateEvent(0, 0, 0, 0);

	while(1)
	{
		unsigned char c;
		DWORD len;

		// Wait RX Event
		WaitCommEvent(handles->serial, &dwEventMask, &ov);
		WaitForSingleObject(ov.hEvent, INFINITE);

		while(ReadFile(handles->serial, &c, 1, &len, &ov), len > 0)
		{
			unsigned int isChar = 1;
			switch(c)
			{
			  case SLIP_END:
				if(packetLength > 0) {
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

			if(isChar)
			{
				packet[packetLength++] = c;
			}

			if(ended == 1)
			{
				utun_write(handles->pTun, packet, packetLength);
				#ifdef _DEBUG
						printf("Written %d bytes to TUN\n", packetLength);
				#endif
				ended = 0;
				packetLength = 0;
			}
		}
	}
}


int main(int argc, char * argv[])
{
	utun tunInterface;
	HANDLE tunReceiveThreadID, serialReceiveThreadID;
	HANDLE serialInterface;
	DCB serialState;
	commHandles handles;

	printf("Smews Pan Manager for windows\n");
	printf("Initializing ... \n");

	// Create TUN interface
	if(utun_open(&tunInterface, IP_FROM, IP_TO, DEV_MTU) == -1)
	{	
		printf("Tun HS\n");
		exit(-1);
	}
	else
		printf("Tun OK, remote ip = %s, MTU = %d\n", IP_TO, DEV_MTU);

	// Open Serial interface
	serialInterface = CreateFile(COM_PORT, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if(serialInterface != INVALID_HANDLE_VALUE)
		printf("Serial OK : %s\n", COM_PORT);
	else
	{
		char lastError[1024];
		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			lastError,
			1024,
			NULL);
		printf("Serial HS : %s", lastError);
		printf("Press a key to terminate...\n");
		getc(stdin);
		utun_close(&tunInterface);
		exit(-1);
	}

	// Configure Serial interface
	GetCommState(serialInterface, &serialState);
	serialState.BaudRate = 115200;
	serialState.ByteSize = 8;
	serialState.Parity = NOPARITY;
	serialState.StopBits = ONESTOPBIT;
	SetCommState(serialInterface, &serialState);
	SetCommMask(serialInterface, EV_RXCHAR | EV_TXEMPTY);

	handles.pTun = &tunInterface;
	handles.serial = serialInterface;

	// Spawn Tun receive thread
	tunReceiveThreadID = CreateThread(NULL, 0, TunReceiveThread, &handles, 0, NULL);

	// Spawn Serial receive thread
	serialReceiveThreadID = CreateThread(NULL, 0, SerialReceiveThread, &handles, 0, NULL);

	printf("PAN manager ready\n");
	printf("Press a key to terminate...\n");
	getc(stdin);

	// Terminate Tun Thread
	TerminateThread(tunReceiveThreadID, 0);

	// Terminate Serial Thread
	TerminateThread(serialReceiveThreadID, 0);

	// Close Tun
	utun_close(&tunInterface);

	// Close Serial
	CloseHandle(serialInterface);

	return 0;
}

