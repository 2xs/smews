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
*/
#ifndef __IP_H__
#define __IP_H__
#include <stdint.h>


#ifdef IPV6
#define PROTO_IP_HLEN   40
#else
#define PROTO_IP_HLEN   20
#endif

#ifdef IPV6
#define PROTO_IP  0x86dd
#else
#define PROTO_IP  0x0800
#endif

#define PROTO_IP_SRCIP_OFFSET 12
#define PROTO_IP_DSTIP_OFFSET 16


/** Gets the destination IP from a IP packet.
 * @param [in] data pointer to the IP header of the packet
 * @param [out] ip pointer to a buffer to store the ip
 */
extern void proto_ip_get_dst(const uint8_t *data, unsigned char *ip);

/** Gets the source IP from a IP packet.
 * @param [in] data pointer to the IP header of the packet
 * @param [out] ip pointer to a buffer to store the ip
 */
extern void proto_ip_get_src(const uint8_t *data, unsigned char *ip);

/** Gets the IP packet size from the IP data.
 * It includes the header size (in v4 AND in v6).
 * @param [in] data pointer to the IP header of the packet
 */
extern uint16_t proto_ip_get_size(const uint8_t *data);

#endif
