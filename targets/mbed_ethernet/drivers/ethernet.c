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
#include "ethernet.h"


void proto_eth_demangle(EthHead *eh, const uint8_t *data)
{
    int idx = 0;
    /* Dst Address */
    GET_MAC(eh->dst.addr, data, idx);
    /* Source Address */
    GET_MAC(eh->src.addr, data, idx);
    /* Type */
    GET_TWO(eh->type, data, idx);
}
void proto_eth_mangle(EthHead *eh, uint8_t *data)
{
    int idx = 0;
    /* Dst Address */
    PUT_MAC(data, idx, eh->dst.addr);
    /* Source Address */
    PUT_MAC(data, idx, eh->src.addr);
    /* Type */
    PUT_TWO(data, idx, eh->type);
}

int proto_eth_addr_equal(EthAddr *a1, EthAddr *a2)
{
    return a1->addr[0] == a2->addr[0] &&
	a1->addr[1] == a2->addr[1] &&
	a1->addr[2] == a2->addr[2] &&
	a1->addr[3] == a2->addr[3] &&
	a1->addr[4] == a2->addr[4] &&
	a1->addr[5] == a2->addr[5];
}

int proto_eth_addr_is_multicast(EthAddr *a)
{
	return a->addr[0] & 1;
}