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
*/

#ifndef UTUN_H
#define UTUN_H 1

#ifdef WIN32
#include "wintun.h"
#elif defined LINUX || defined FREEBSD
#include "nixtun.h"
#else
#error Your system isn't supported by libutun, but you can help us to do it. Thanks.
#endif

//Portable functions definitions
/**
Open a new TUN device.<br>
The <b>utun</b> structure is platform specific and should not be used directly in a <b>portable</b> program.<br>
<code>
utun t;
utun_open(&t,"192.168.1.5","192.168.1.6");
</code>
*/
extern int  utun_open  (utun *t, char *ip_src, char *ip_dest, unsigned long mtu);

/**
Write a paquet on an opened device.
*/
extern int  utun_write (utun *t, unsigned char *buf, int len);

/**
Read a paquet from an opened device.
*/
extern int  utun_read  (utun *t, unsigned char *buf, int len);

/**
Close an opened device.
*/
extern void utun_close (utun *t);

/**
Test an opened device for bytes availablity.
*/
extern int  utun_select(utun *t);// do not work on all platform

#endif

