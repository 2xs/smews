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
/*
<generator>
        <handlers doPacketIn="icmp4_packet_in" doPacketOut="icmp4_packet_out"/>
        <properties protocol="1" />
</generator>
 */

#ifdef IPV6
	#error "This application can only work for IPv4 smews"
#endif

#ifdef DISABLE_GP_IP_HANDLER
	#error "This application can not work with disable_general_purpose_ip_handler"
#endif

static char icmp_payload[OUTPUT_BUFFER_SIZE];
static int buffer_size;
static char sequence_number[2];
static char identifier[2];
static char checksum[2];

#define ICMP_ECHO_REQUEST 	8
#define ICMP_ECHO_REPLY		0
#define ICMP_HEADER_SIZE	8

char icmp4_packet_in(const void *connection_info)
{
	uint8_t tmp;
	uint16_t payload_size = get_payload_size(connection_info);
	int i;

	checksum_init();
	tmp = in(); /* type */

	if (tmp != ICMP_ECHO_REQUEST)
		return 0; /* drop packet and do not generate a reply */
	checksum_add(tmp);
	tmp = in(); /* code */
	checksum_add(tmp);

	checksum[S0] = in();
	checksum[S1] = in();
	checksum_add16(UI16(checksum));

	identifier[S0] = in();
	identifier[S1] = in();
	checksum_add16(UI16(identifier));

	sequence_number[S0] = in();
	sequence_number[S1] = in();
	checksum_add16(UI16(sequence_number));
	for (i = 0 ; i < payload_size - ICMP_HEADER_SIZE; ++i)
	{
		icmp_payload[i] = in();
		checksum_add(icmp_payload[i]);
	}
	buffer_size = payload_size - ICMP_HEADER_SIZE;
	checksum_end();
	if(UI16(current_checksum) != 0xffff)
		return 0; /* invalid checksum */
	return 1;
}

char icmp4_packet_out(const void *connection_info)
{
	int i;
	/* generate reply */

	out_c(ICMP_ECHO_REPLY); /* type */
	out_c(0);  /* code */

	/* Generate checksum from request one, only the type byte has change,
	 * so the value can be reused to accelerate the computation of the checksum
	 */
	UI16(checksum) += (ICMP_ECHO_REQUEST << 8);
	if ((UI16(checksum) >> 8) < ICMP_ECHO_REQUEST) /* overflow, should add one */
		UI16(checksum)++;
	out_c(checksum[S0]);
	out_c(checksum[S1]);
	out_c(identifier[S0]);
	out_c(identifier[S1]);
	out_c(sequence_number[S0]);
	out_c(sequence_number[S1]);
	for (i = 0 ; i < buffer_size ; ++i)
	{
		out_c(icmp_payload[i]);
	}
	return 0;
}
