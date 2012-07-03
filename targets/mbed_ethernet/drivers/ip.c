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
  Created: 2011-08-31
  Time-stamp: <2011-09-02 10:51:03 (hauspie)>
*/
#include "types.h"
#include "protocols.h"
#include "ip.h"


#ifdef IPV6
#define IP_DST_OFFSET 24
#define IP_SRC_OFFSET 8
#define IP_SIZE_OFFSET 4
#define GET_IP(ip,data,idx) do {int i; for (i =0 ; i < 16 ; ++i) (ip)[15-i] = (data)[(idx)+i];} while(0)
#else
#define IP_DST_OFFSET 16
#define IP_SRC_OFFSET 12
#define IP_SIZE_OFFSET 2
#define GET_IP(ip, data, idx) GET_FOUR(UI32(ip), data, idx)
#endif

void proto_ip_get_dst(const uint8_t *data, unsigned char *ip)
{
    int idx = IP_DST_OFFSET;
    GET_IP(ip, data, idx);
}
void proto_ip_get_src(const uint8_t *data, unsigned char *ip)
{
    int idx = IP_SRC_OFFSET;
    GET_IP(ip, data, idx);
}

uint16_t proto_ip_get_size(const uint8_t *data)
{
    int idx = IP_SIZE_OFFSET;
    uint16_t size;
    GET_TWO(size, data, idx);
#ifdef IPV6
	size += PROTO_IP_HLEN;
#endif
    return size;
}