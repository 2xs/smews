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
#include "utun.h"

/**

usage :
launch utuntest in a shell
open a new shell, try to ping 192.168.1.6, let see what happen
NB : this sample is pre-configurated with Win32 limitations, see wintun.c
*/

#define LOCAL_MTU 255

static void print_packet(const unsigned char *ptr,int len);

int main(int argc, char *argv[])
{
    utun t;

    if(utun_open(&t,"192.168.1.5","192.168.1.6",LOCAL_MTU)==0)
    {
        unsigned char paquet[LOCAL_MTU];
        int len;

        memset(paquet,0,LOCAL_MTU);

        while((len= utun_read(&t,paquet,LOCAL_MTU))>-1)
        {
            if(len==0)
            {
                fprintf(stderr,"Why should I receive 0 bytes ?\n");
                continue;
            }
            print_packet(paquet,len);
            if(len)
            {
                //echo-reply
                utun_write(&t,paquet,len);
            }
        }

        utun_close(&t);
    }

    return 0;
}

void print_packet(const unsigned char *ptr,int len)
{
    int i,j;

    printf("Dumping %dbytes\n",len);
    for(i=0;i<len;i++)
    {
        printf("%.4X: ",i);
        for(j=0;i+j<len && j<16;j++)
        {
            printf("%.2X",ptr[i+j]);
        }
        if(j<16)
        {
            for(;j<16;j++)
                printf("  ");
        }
        printf("  ");
        for(j=0;i+j<len && j<16;j++)
        {
            printf("%c",isgraph(ptr[i+j])?ptr[i+j]:'.');
        }
        if(j<16)
        {
            for(;j<16;j++)
                printf(" ");
        }
        printf("\n");        
        
        i+=15;
    }
    printf("\n");        
    fflush(stdout);

}

