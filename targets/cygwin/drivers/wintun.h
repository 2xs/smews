/*
*Lib Universal TUN (libutun) Copyright(C) Geoffroy Cogniaux < geoffroy@cogniaux.com >
*
*Lib Universal TUN is free software; you can redistribute it and / or
*modify it under the terms of the GNU General Public
*License as published by the Free Software Foundation; either
*version 2.1 of the License, or(at your option)any later version.
*
*Lib Universal TUN is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*General Public License for more details.
*
*You should have received a copy of the GNU General Public
*License along with Lib Universal TUN; if not, write to the Free Software
*Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA
*
**
*Lib Universal TUN uses TAP-Win32 on Windows(r)
* -- A kernel driver to provide virtual tap device
*             functionality on Windows.  Originally derived
*             from the CIPE-Win32 project by Damion K. Wilson,
*             with extensive modifications by James Yonan.
*
*All original source code using TAP-Win32 which derives from the CIPE-Win32 project is
*Copyright (C) Damion K. Wilson, 2003, and is released under the
*GPL version 2.
*
*All other original source code using TAP-Win32 is 
*Copyright (C) 2002-2006 OpenVPN Solutions LLC,
*and is released under the GPL version 2 .
*/

#ifndef WINTUN_H
#define WINTUN_H 1

#ifdef WIN32
#undef UNICODE
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <string.h>

#define itoa _itoa
#define snprint _snprintf

//some decl avoiding winsock import, differents names are requiered because accessibles via windows.h in VC++
extern unsigned short utun_htons(unsigned short n);
extern unsigned short utun_ntohs(unsigned short n);
extern unsigned long utun_htonl(unsigned long n);
extern unsigned long utun_ntohl(unsigned long n);
extern unsigned long utun_inet_addr(const char *src);

#if defined (__CYGWIN__)
#include <ctype.h>
struct in_addr 
{
  union 
  {
    struct { unsigned char s_b1, s_b2, s_b3, s_b4; } s_un_b;
    struct { unsigned short s_w1, s_w2; } s_un_w;
    unsigned long s_addr;
  };
};
#endif

typedef struct tun_t {
	HANDLE device_handle;
	unsigned int ip_src,ip_dest;
	OVERLAPPED overlap_read, overlap_write;
} utun;


//Following come from OpenVpn
#define TAP_CONTROL_CODE(request,method) \
  CTL_CODE (FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)

#define TAP_IOCTL_GET_MAC               TAP_CONTROL_CODE (1, METHOD_BUFFERED)
#define TAP_IOCTL_GET_VERSION           TAP_CONTROL_CODE (2, METHOD_BUFFERED)
#define TAP_IOCTL_GET_MTU               TAP_CONTROL_CODE (3, METHOD_BUFFERED)
#define TAP_IOCTL_GET_INFO              TAP_CONTROL_CODE (4, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_POINT_TO_POINT TAP_CONTROL_CODE (5, METHOD_BUFFERED)
#define TAP_IOCTL_SET_MEDIA_STATUS      TAP_CONTROL_CODE (6, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_MASQ      TAP_CONTROL_CODE (7, METHOD_BUFFERED)
#define TAP_IOCTL_GET_LOG_LINE          TAP_CONTROL_CODE (8, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_SET_OPT   TAP_CONTROL_CODE (9, METHOD_BUFFERED)

#define ADAPTER_KEY "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"
#define NETWORK_CONNECTIONS_KEY "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}"

#define USERMODEDEVICEDIR "\\\\.\\Global\\"
#define SYSDEVICEDIR      "\\Device\\"
#define USERDEVICEDIR     "\\DosDevices\\Global\\"
#define TAPSUFFIX         ".tap"

#define TAP_COMPONENT_ID "tap0801"

#endif

extern int  utun_open  (utun *t, char *ip_src, char *ip_dest, unsigned long mtu);
extern int  utun_write (utun *t, unsigned char *buf, int len);
extern int  utun_read  (utun *t, unsigned char *buf, int len);
extern void utun_close (utun *t);
extern int  utun_select(utun *t);

#endif
