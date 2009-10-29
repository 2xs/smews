#include "types.h"
#undef UNICODE
#include <windows.h>
#include "wintun.h"
#include "target.h"

utun t;
#define BUF_SIZE DEV_MTU
unsigned char read_buf[BUF_SIZE];
unsigned char write_buf[BUF_SIZE];
int STATE_READ,STATE_WRITE,MAX_READ,MAX_WRITE;
short r;

void hardware_init(void)
{
#if (DEFAULT_IP_ADDR_0 != 192 || DEFAULT_IP_ADDR_1 != 168 || DEFAULT_IP_ADDR_2 != 0 || DEFAULT_IP_ADDR_3 != 2)
#error smews IP Address has to be 192.168.0.2, @see wintun.c : changes can be done in config.h
#endif
    utun_open(&t,"192.168.0.1", "192.168.0.2", DEV_MTU );
    STATE_READ= 0;
    STATE_WRITE= 0;
    MAX_READ= 0;
    MAX_WRITE= -1;
    memset(read_buf,0,BUF_SIZE);
    memset(write_buf,0,BUF_SIZE);
}

unsigned char dev_get(void)
{
    r++;

    if(STATE_READ == MAX_READ)
	{
        MAX_READ= utun_read(&t,read_buf,BUF_SIZE);
        STATE_READ= 0;
    }
    return read_buf[STATE_READ++];
}

void dev_put(unsigned char byte)
{
    if(STATE_WRITE+1 == MAX_WRITE)
	{
        if(MAX_WRITE>0)
        {
            int written;
            write_buf[STATE_WRITE++]= byte;
            written= utun_write(&t,write_buf,MAX_WRITE);
            if(written!=MAX_WRITE)
                abort();
        }
        else
            abort();
    }
    else
        write_buf[STATE_WRITE++]= byte;
}

char dev_prepare_output(uint16_t len)
{
    if(len>BUF_SIZE)
    {
        //MSS too large !?!
        abort();
        return 0;
    }
    fflush(stdout);
    memset(write_buf,0,BUF_SIZE);
    STATE_WRITE= 0;
    MAX_WRITE= len;
    return 1;
}

unsigned char dev_data_to_read(void)
{
    return MAX_READ - STATE_READ;
}
