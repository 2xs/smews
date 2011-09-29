/*
* Copyright or © or Copr. 2011, Michael Hauspie
* 
* Author e-mail: michael.hauspie@lifl.fr
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
/*
  Author: Michael Hauspie <michael.hauspie@univ-lille1.fr>
  Created: 2011-08-30
  Time-stamp: <2011-09-29 10:29:55 (hauspie)>

*/
#include <rflpc17xx/debug.h>
#include "protocols.h"
#include "hardware.h"

#include "mbed_debug.h"

#define MAX_ARP_ENTRY 10

/*#define DUMP_CACHE*/

typedef struct
{
    uint32_t ip;
    uint32_t timestamp;
    EthAddr mac;
    /* TODO, add a timestamp to choose the couple to delete from cache */
} ArpEntry;

ArpEntry _arp_table[MAX_ARP_ENTRY];

#define GET_BYTE(ptr,i) (((uint8_t*)(ptr))[i])

void _dump_arp_cache()
{
    int i;
    MBED_DEBUG("ARP CACHE DUMP\r\n");
    for (i = 0 ; i < MAX_ARP_ENTRY ; ++i)
    {
	if (_arp_table[i].ip != 0)
	{
	    MBED_DEBUG("%d: %02x:%02x:%02x:%02x:%02x:%02x -> %d.%d.%d.%d (ts: %d ms)\r\n", i,
		       _arp_table[i].mac.addr[0],
		       _arp_table[i].mac.addr[1],
		       _arp_table[i].mac.addr[2],
		       _arp_table[i].mac.addr[3],
		       _arp_table[i].mac.addr[4],
		       _arp_table[i].mac.addr[5],
		       GET_BYTE(&_arp_table[i].ip, 3),
		       GET_BYTE(&_arp_table[i].ip, 2),
		       GET_BYTE(&_arp_table[i].ip, 1),
		       GET_BYTE(&_arp_table[i].ip, 0), mbed_get_time());
	}
	else
	    MBED_DEBUG("%d: empty\r\n", i);

    }
    MBED_DEBUG("END ARP CACHE DUMP\r\n");
}

void arp_add_mac(uint32_t ipv4, EthAddr *mac)
{
    int i, min_time_idx = 0;
    uint32_t min_time;
    
    min_time = _arp_table[0].timestamp;

    for (i = 0 ; i < MAX_ARP_ENTRY ; ++i)
    {
	if (_arp_table[i].timestamp < min_time)
	{
	    min_time = _arp_table[i].timestamp;
	    min_time_idx = i;
	}
	if (_arp_table[i].ip == ipv4 || _arp_table[i].ip == 0)
	{
	    _arp_table[i].mac = *mac;
	    _arp_table[i].timestamp = mbed_get_time();
	    if (_arp_table[i].ip == 0)
	    {
		_arp_table[i].ip = ipv4;
#ifdef DUMP_CACHE
		_dump_arp_cache();
#endif
	    }
	    return;
	}
    }
    /* every entry is used, remove the oldest one */
    _arp_table[min_time_idx].ip = ipv4;
    _arp_table[min_time_idx].mac = *mac;
#ifdef DUMP_CACHE
    _dump_arp_cache();
#endif
}

int arp_get_mac(uint32_t ipv4, EthAddr *mac)
{
    int i;
    for (i = 0 ; i < MAX_ARP_ENTRY ; ++i)
    {
	if (_arp_table[i].ip == ipv4)
	{
	    *mac = _arp_table[i].mac;
	    return 1;
	}
    }
    return 0;
}
