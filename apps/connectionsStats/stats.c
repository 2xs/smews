/*
* Copyright or © or Copr. 2008, Simon Duquennoy
*
* Author e-mail: simon.duquennoy@lifl.fr
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
	<handlers doGet="doGet"/>
	<properties persistence="volatile"/>
</generator>
*/

#include "generators.h"
#include "connections.h"

static char doGet(struct args_t *args) {
	int cpt = 0;
	FOR_EACH_CONN(conn, {
		out_str("Connection: "); out_uint(cpt++); out_str("\n");
		if (IS_HTTP(conn))
		{
			out_str("\tport: "); out_uint(UI16(conn->protocol.http.port)); out_str("\n");
			out_str("\ttcp_state: "); out_uint(conn->protocol.http.tcp_state); out_str("\n");
		}
#ifndef DISABLE_GP_IP_HANDLER
		else if (IS_GPIP(conn))
		{
			out_str("\tGPIP for protocol "); out_uint(conn->output_handler->handler_data.generator.handlers.gp_ip.protocol); out_str("\n");
		}
#endif
		out_str("\toutput_handler: ");
		if(conn->output_handler)
			out_str("****\n");
		else
			out_str("NULL\n");
		out_str("\tsomething to send: "); out_uint(something_to_send(conn)); out_str("\n");
	})
	return 1;
}
