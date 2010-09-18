
#include "scard.h"

int open_tunnel(utun *tunnel)
{
    return utun_open(tunnel,"192.168.1.5", "192.168.1.6",MTU);
}

void close_tunnel(utun *tunnel)
{
    utun_close(tunnel);
}
