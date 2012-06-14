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
<generator>
        <handlers doPacketIn="icmp4_packet_in" doPacketOut="icmp4_packet_out"/>
        <properties protocol="1" />
</generator>
 */
#include <rflpc17xx/rflpc17xx.h>

char icmp4_packet_in(uint8_t protocol, uint16_t payload_size)
{
/*	printf("icmp_packet_in called with %d as protocol number\r\n", protocol); */
	while (payload_size)
	{
		/* printf("%02x\r\n", (uint8_t) in()); */
		payload_size--;
	}
	return 1;
}

char icmp4_packet_out(uint8_t protocol)
{
/*	printf("icmp_packet_out called with %d as protocol number\r\n", protocol); */
	return 0;
}
