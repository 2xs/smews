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

#if defined LINUX || defined FREEBSD
#include "utun.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#define TUN_MTU 1500
#define TUN_DEV "/dev/net/tun"
#define IFCONFIG "ifconfig %s %s pointopoint %s mtu %d"

int  utun_open  (utun *t, char *ip_src, char *ip_dest, unsigned long mtu)
{
    struct ifreq ifr;
    int fd, err;
    char buf[256] ;
    
    if( (fd = open(TUN_DEV, O_RDWR)) < 0 ) 
    {
        printf("Unable to open %s\n",TUN_DEV);
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TUN|IFF_NO_PI ; 

    if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 )
    {
        close(fd);
        return -1;
    }
    //printf("Device used for tunnel : %s \n",ifr.ifr_name);
    
    snprintf(buf,256, IFCONFIG, ifr.ifr_name, ip_src, ip_dest, mtu>100?mtu:TUN_MTU);
    system(buf);
#ifdef FREEBSD
    if( fd > -1 )
    {
       int i=0;
       /* Disable extended modes */
       ioctl(fd, TUNSLMODE, &i);	
       i = 1;
       ioctl(fd, TUNSIFHEAD, &i);
    }
#endif
    t->fd= fd;
    return fd;
}

int  utun_write (utun *t, unsigned char *buf, int len)
{
    return write(t->fd,buf,len);
}

int  utun_read  (utun *t, unsigned char *buf, int len)
{
    return read(t->fd,buf,len);
}

void utun_close (utun *t)
{
    close(t->fd);
    t->fd= -1;
}

int  utun_select(utun *t)
{
    struct timeval tv;
    fd_set fd_readset;    
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fd_readset);
    FD_SET(t->fd, &fd_readset);
    return select(t->fd + 1, &fd_readset, NULL, NULL, &tv);
}


#endif
