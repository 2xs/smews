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
        <handlers doPacketIn="icmp6_packet_in" doPacketOut="icmp6_packet_out"/>
        <properties protocol="58" />
</generator>
 */

#ifndef IPV6
	#warning "icmpv6 application can only work for IPv6 smews, disabling it"
#define DISABLE_ICMPV6
#endif

#ifdef DISABLE_GP_IP_HANDLER
	#error "icmpv6 application can not work with disable_general_purpose_ip_handler, disabling it"
#endif

#if !defined(LINK_LAYER_ADDRESS) || !defined(LINK_LAYER_ADDRESS_SIZE)
	#warning "icmpv6 application can not work without LINK_LAYER_ADDRESS and LINK_LAYER_ADDRESS_SIZE defined in your target, disabling it"
#define DISABLE_ICMPV6
#endif


#ifdef DISABLE_ICMPV6
/* Just implement empty callbacks for link purpose */
char icmp6_packet_in(const void *connection_info){return 0;}
char icmp6_packet_out(const void *connection_info){return 0;}
#else

#define ICMP_ECHO_REQUEST 			128
#define ICMP_ECHO_REPLY				129
#define ICMP_NEIGHBOR_SOLICITATION 	135
#define ICMP_NEIGHBOR_ADVERTISEMENT	136

char icmp6_send_na(const void *connection_info);

char icmp6_decode_ns(const void *connection_info)
{
	unsigned char target_ip[16];
	unsigned char local_ip[16];
	int i;

	/* Code */
	in();

	/* Checksum */
	in();in();

	/* Four reserved bytes */
	in();in();in();in();

	/* Target address */
	for (i = 0 ; i < 16 ; ++i)
		target_ip[15-i] = in();

	/* Source link address */
	for (i = 0 ; i < LINK_LAYER_ADDRESS_SIZE ; ++i)
		in();

	/* check if request is for me */
	get_local_ip(connection_info, local_ip);
	for (i = 0 ; i < 16 ; ++i)
		if (local_ip[i] != target_ip[i])
			return 0;

	/* request for me, have to answer */
	return ICMP_NEIGHBOR_SOLICITATION;
}

char icmp6_packet_in(const void *connection_info)
{
	uint8_t type;

	checksum_init();
	type = in(); /* Get ICMP type */
	checksum_add(type);

	switch (type)
	{
		case ICMP_ECHO_REQUEST:
			return 0;
		case ICMP_NEIGHBOR_SOLICITATION:
			return icmp6_decode_ns(connection_info);
		default:
			break;
	}
	return 0;
}

char icmp6_packet_out(const void *connection_info)
{
	switch (get_send_code(connection_info))
	{
		case ICMP_NEIGHBOR_SOLICITATION:
			return icmp6_send_na(connection_info);
	}
	return 0;
}


/* Generates a Neighbor Advertisement packet */
char icmp6_send_na(const void *connection_info)
{
	unsigned char local_ip[16];
	unsigned char remote_ip[16];
	int i;

	checksum_init();
	/* First calculate the checksum of the pseudo header */
	get_remote_ip(connection_info, remote_ip);
	get_local_ip(connection_info, local_ip);

	for (i = 0 ; i < 16 ; ++i)
		checksum_add(local_ip[15-i]);
	for (i = 0 ; i < 16 ; ++i)
		checksum_add(remote_ip[15-i]);

	/* Packet length */
	/* 8: type+code+checksum+flags+reserved */
	/* 16: target address */
	/* LINK_LAYER_ADDRESS_SIZE + 2: target link address */
	checksum_add16(0);
	checksum_add16(LINK_LAYER_ADDRESS_SIZE + 2 + 16 + 8);

	checksum_add16(0);
	checksum_add(0);
	checksum_add(58); /* ICMPv6 */
	/* This ends the ipv6 pseudo header checksum calculation */

	/*******************************************/
	/* Type */
	checksum_add(ICMP_NEIGHBOR_ADVERTISEMENT);

	/*******************************************/
	/* Code */
	checksum_add(0);

	/*******************************************/
	/* R/S/O bits*/
	checksum_add(0x60);

	/*******************************************/
	/* reserved */
	checksum_add(0);
	checksum_add16(0);

	/*******************************************/
	/* local_ip */
	for (i = 0 ; i < 16 ; ++i)
		checksum_add(local_ip[15-i]);

	/*******************************************/
	/* my link layer */
	checksum_add(2); /* Option type: target link layer address */
	checksum_add((LINK_LAYER_ADDRESS_SIZE + 2)>>3); /* The size is in unit of 8 bytes and includes type and length fields */
	for (i = 0 ; i < LINK_LAYER_ADDRESS_SIZE ; ++i)
		checksum_add(LINK_LAYER_ADDRESS[i]);
	checksum_end();
	/* generate response */

	/*******************************************/
	/* type */
	out_c(ICMP_NEIGHBOR_ADVERTISEMENT);

	/*******************************************/
	/* code */
	out_c(0);

	/*******************************************/
	/* checksum */
	UI16(current_checksum) = ~UI16(current_checksum);
	out_c(current_checksum[S0]);
	out_c(current_checksum[S1]);

	/*******************************************/
	/* R/S/O bits */
	out_c(0x60);
	/* 3 more reserved bytes */
	out_c(0);out_c(0);out_c(0);

	/*******************************************/
	/* My ip */
	for (i = 0 ; i < 16 ; ++i)
		out_c(local_ip[15-i]);

	/*******************************************/
	/* my link layer */
	out_c(2); /* Option type: target link layer address */
	out_c((LINK_LAYER_ADDRESS_SIZE+2)>>3); /* The size is in unit of 8 bytes and includes type and length fields */
	for (i = 0 ; i < LINK_LAYER_ADDRESS_SIZE ; ++i)
		out_c(LINK_LAYER_ADDRESS[i]);
	return 0;
}
#endif
