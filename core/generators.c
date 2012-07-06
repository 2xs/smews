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

#include "generators.h"
#include "output.h"
#include "connections.h"
#include "memory.h"

#ifndef DISABLE_COMET
/*-----------------------------------------------------------------------------------*/
char server_push(const struct output_handler_t *push_handler /*CONST_VAR*/) {

	if(push_handler->handler_comet) {
		FOR_EACH_CONN(conn, {
			if(IS_HTTP(conn) && conn->output_handler == push_handler) {
				conn->protocol.http.comet_send_ack = 0;
				UI32(conn->protocol.http.final_outseqno) = UI32(conn->protocol.http.next_outseqno) - 1;
			}
		})
		return 1;
	} else {
		return 0;
	}
}
#endif

/*-----------------------------------------------------------------------------------*/
void out_uint(uint16_t i) {
#ifndef DISABLE_POST
	/* unauthorised out */
	if(coroutine_state.state == cor_in)
		return;
#endif
	char buffer[6];
	char *c = buffer + 5;
	buffer[5] = '\0';
	do {
		*--c = (i % 10) + '0';
		i /= 10;
	} while(i);
	while(*c) {
		out_c(*c++);
	}
}

/*-----------------------------------------------------------------------------------*/
void out_str(const char str[]) {
#ifndef DISABLE_POST
	/* unauthorised out */
	if(coroutine_state.state == cor_in)
		return;
#endif
	const char *c = str;
	while(*c) {
		out_c(*c++);
	}
}
