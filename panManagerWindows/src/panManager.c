#include "wintun.h"

#define DEV_MTU 1500
#define IP_FROM "192.168.2.1"
#define IP_TO   "192.168.2.2"

#define COM_PORT "COM6"

/* SLIP special bytes */
#define SLIP_END             0xC0    /* indicates end of packet */
#define SLIP_ESC             0xDB    /* indicates byte stuffing */
#define SLIP_ESC_END         0xDC    /* ESC ESC_END means END data byte */
#define SLIP_ESC_ESC         0xDD    /* ESC ESC_ESC means ESC data byte */

typedef struct {
	utun * pTun;
	HANDLE serial;
} commHandles;

DWORD WINAPI TunReceiveThread(LPVOID lpParam)
{
	commHandles * handles = (commHandles*) lpParam;
	unsigned char packet[DEV_MTU];
	unsigned char packetSlip[DEV_MTU*2];

	while(1)
	{
		unsigned int nbBytesFromTun;
		unsigned int nbBytesToSerial;
		unsigned int i;
		unsigned char * pPacket;
		DWORD nbBytesWrittenToSerial;

		// Read a packet from TUN
		nbBytesFromTun = utun_read(handles->pTun, packet, DEV_MTU);

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
		WriteFile(handles->serial, packetSlip, nbBytesToSerial, &nbBytesWrittenToSerial, NULL);

		if(nbBytesToSerial != nbBytesWrittenToSerial)
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
			printf("Error on serial write : %s", lastError);
		}
	}
}

DWORD WINAPI SerialReceiveThread(LPVOID lpParam)
{
	commHandles * handles = (commHandles*) lpParam;
	unsigned char packet[DEV_MTU];
	unsigned int packetLength = 0;
	unsigned int ended = 0;
    enum slip_state_e {slip_in,slip_escaped};
    /* current slip state */
    volatile enum slip_state_e slip_state = slip_in;

	while(1)
	{
		unsigned char c;
		DWORD len;
		unsigned int isChar = 1;
		unsigned int r;
		r = ReadFile(handles->serial, &c, 1, &len, NULL);
		if(len)
		{
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
	serialInterface = CreateFile(COM_PORT, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
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

