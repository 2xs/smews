/*
* Copyright or © or Copr. 2012, Michael Hauspie
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
#ifdef ENABLE_LL_CACHE

#include "target.h"
#include "connections.h"

#ifndef LINK_LAYER_CACHE_MAX_ENTRY
#define LINK_LAYER_CACHE_MAX_ENTRY 10
#endif

/*#define DUMP_CACHE*/

typedef struct
{
#ifdef IPV6
	unsigned char ip[16];
#else
    unsigned char ip[4];
#endif
    uint32_t timestamp;
    unsigned char link_layer[LINK_LAYER_ADDRESS_SIZE];
} LinkLayerEntry;

static LinkLayerEntry _link_layer_table[LINK_LAYER_CACHE_MAX_ENTRY];

#ifdef TIME_MILLIS
#define NOW (TIME_MILLIS)
#else
static uint32_t _current_time = 0;
#define NOW (++_current_time)
#endif

#ifndef LINK_LAYER_ADDRESS_SIZE
#error "The target MUST define LINK_LAYER_ADDRESS_SIZE for this cache to work"
#endif

#ifdef IPV6
static inline int _ip_equal(const unsigned char *ip1, const unsigned char *ip2)
{
	uint8_t i;
	for (i = 0 ; i < 16; ++i)
	    if (ip1[i] != ip2[i])
		return 0;
	return 1;
}
static inline int _ip_is_null(const unsigned char *ip)
{
	uint8_t i;
	for (i = 0 ; i < 16 ; ++i)
	{
	    if (ip[i])
		return 0;
	}
	return 1;
}
#define IP_EQUAL(ip1, ip2) _ip_equal(ip1,ip2)
#define IP_IS_NULL(ip) _ip_is_null(ip)
#define COPY_IP(ip1,ip2) memcpy(ip1,ip2,16)
#else
#define IP_EQUAL(ip1, ip2) (UI32(ip1) == UI32(ip2))
#define IP_IS_NULL(ip) (UI32(ip) == 0)
#define COPY_IP(ip1, ip2) UI32(ip1) = UI32(ip2)
#endif


void add_link_layer_address(unsigned char *ip, unsigned char *link_layer)
{
    uint8_t i, min_time_idx = 0;
    uint32_t min_time;

    min_time = _link_layer_table[0].timestamp;

    for (i = 0 ; i < LINK_LAYER_CACHE_MAX_ENTRY ; ++i)
    {
	if (_link_layer_table[i].timestamp < min_time)
	{
	    min_time = _link_layer_table[i].timestamp;
	    min_time_idx = i;
	}
	if (IP_EQUAL(_link_layer_table[i].ip,ip) || IP_IS_NULL(_link_layer_table[i].ip))
	{
	    memcpy(_link_layer_table[i].link_layer, link_layer, LINK_LAYER_ADDRESS_SIZE);
	    _link_layer_table[i].timestamp = NOW;
	    if (IP_IS_NULL(_link_layer_table[i].ip))
	    {
		COPY_IP(_link_layer_table[i].ip,ip);
	    }
	    return;
	}
    }
    /* every entry is used, remove the oldest one */
    COPY_IP(_link_layer_table[min_time_idx].ip,ip);
    memcpy(_link_layer_table[min_time_idx].link_layer, link_layer, LINK_LAYER_ADDRESS_SIZE);
}

int get_link_layer_address(const unsigned char *ip, unsigned char *link_layer)
{
    uint8_t i;
    for (i = 0 ; i < LINK_LAYER_CACHE_MAX_ENTRY ; ++i)
    {
		if (IP_EQUAL(_link_layer_table[i].ip,ip))
		{
			memcpy(link_layer, _link_layer_table[i].link_layer, LINK_LAYER_ADDRESS_SIZE);
			return 1;
		}
    }
    return 0;
}

#endif
