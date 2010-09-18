
#ifndef SCARD_H
#define SCARD_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utun/utun.h"

#ifdef WIN32
    #undef UNICODE
    #include <windows.h>
    #include <winscard.h>
    #include "win32/winip.h"

    extern char* pcsc_stringify_error(LONG pcscError);
    #define usleep Sleep
    #define sleep(_tt) Sleep((_tt)*1000)
    #define strlcpy strncpy
    #define snprintf _snprintf
    #define MAX_ATR_SIZE 36
    #define MAX_READERNAME 128
    #define MAX_BUFFER_SIZE 255
#else
    #include <ctype.h>
    #include <netinet/ip.h>
    #include "linux/wintypes.h"
    #include "linux/pcsclite.h"
    #include "linux/winscard.h"
#endif

//APDU Status Word wrappers
#define SW2(_sw) (((_sw)>>8) & 0xFF)
#define SW1(_sw) ((_sw) & 0xFF)



#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


#define MTU 1500
#define RECONNECTION_DELAY 3
#define RECONNECTION_COUNT 100

typedef union raw_packet
{
    struct {
        struct iphdr ip;
    } fmt;
    unsigned char raw[MTU+1];
} raw_packet;

//-------------------- terminal.c ---------------------------------
//init a card terminal and wait for a smartcard to be inserted ( T0 protocol, shared mode )
//check if the card is a smews Card with ATR
extern int init_terminal(SCARDCONTEXT *hContext,SCARDHANDLE *hCard);
//disconnect from terminal and currently UNPOWER smartcard too
extern void release_terminal(SCARDCONTEXT *hContext,SCARDHANDLE *hCard);
extern int check_terminal_status(SCARDCONTEXT hContext,SCARDHANDLE hCard);

//-------------------- apdu.c ----------------------
extern short send_apdu(SCARDCONTEXT hContext,SCARDHANDLE hCard,char *paquet,int len,LONG *rv,int P2);      
extern short recv_apdu(SCARDCONTEXT hContext,SCARDHANDLE hCard,utun *t,unsigned short sw,LONG *rv);

//------------------- tunnel.c -----------------------
extern int open_tunnel(utun *tunnel);
extern void close_tunnel(utun *tunnel);

//------------------- utils.c ------------------------
extern void print_packet(const unsigned char *ptr,int len);

//------------------- signal.c -----------------------
extern void init_signals(void);

#endif//SCARD_H
