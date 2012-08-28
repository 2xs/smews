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

#include "connections.h"
#include "memory.h"

/* deallocate all the memory used by in-flight segments that are now acknowledged (with out_seqno < inack) */
void clean_service(struct generator_service_t *service, unsigned char inack[]) {
	struct in_flight_infos_t *last_if_ok = NULL;
	struct in_flight_infos_t *first_if_out = service->in_flight_infos;
	if(first_if_out) {
		/* look for the first in-flight segment to be free'd (segments are sorted) */
		if(inack) {
			while(first_if_out && UI32(first_if_out->next_outseqno) >= UI32(inack)) {
				last_if_ok = first_if_out;
				first_if_out = first_if_out->next;
			}
		}

		/* shorten the list */
		if(last_if_ok) {
			last_if_ok->next = NULL;
		} else {
			service->in_flight_infos = NULL;
		}

		/* free all acknowledged segment information */
		while(first_if_out) {
			struct in_flight_infos_t *next = first_if_out->next;
			if(service->is_persistent) {
				mem_free((void *)first_if_out->infos.buffer, OUTPUT_BUFFER_SIZE);
#ifndef DISABLE_COROUTINES
			} else {
				mem_free((void *)first_if_out->infos.context->stack, first_if_out->infos.context->stack_size);
				mem_free((void *)first_if_out->infos.context, sizeof(struct cr_context_t));
			}
#else
            }
#endif
			mem_free((void *)first_if_out, sizeof(struct in_flight_infos_t));
			first_if_out = next;
		}
	}
}

