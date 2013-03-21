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

#ifndef __SMEWS_H__
#define __SMEWS_H__

#include "input.h"
#include "output.h"
#include "connections.h"
#include "coroutines.h"
#include "timers.h"
#include "memory.h"

extern void smews_retransmit(void);

/* Inlinged because used only once */

/*-----------------------------------------------------------------------------------*/
/* Processes one step */
static inline unsigned char smews_main_loop_step() {
#ifndef DISABLE_TIMERS
	smews_timers();
#endif
	if(!smews_receive() && !smews_send()) {
#ifdef DEV_WAIT
		DEV_WAIT;
#endif
		return 0;
	} else {
		return 1;
	}
}

/*-----------------------------------------------------------------------------------*/
/* Smews initialization */
static inline void smews_init(void) {
	uint16_t x;
	const struct output_handler_t * /*CONST_VAR*/ output_handler;

#ifdef STACK_DUMP
	uint32_t first_var;
	stack_base = (unsigned char *) &first_var;
	for(stack_i = 0; stack_i < STACK_DUMP_SIZE ; stack_i++){
		stack_base[-stack_i] = STACK_MAGIC;
	}
#endif

	mem_reset();

	/* target-dependent inititialization */
	HARDWARE_INIT;

#ifndef DISABLE_TIMERS
	last_transmission_time = 1;
#endif

	x = 0;
	while((output_handler = CONST_ADDR(resources_index[x])) != NULL) {
	    if(CONST_UI8(output_handler->handler_type) == type_generator 
#ifndef DISABLE_GP_IP_HANDLER
				|| CONST_UI8(output_handler->handler_type) == type_general_ip_handler
#endif
				) {
			generator_init_func_t * init_handler = CONST_ADDR(GET_GENERATOR(output_handler).init);
			if(init_handler)
				init_handler();
		}
		x++;
	}
#ifndef DISABLE_TIMERS
	set_timer(&smews_retransmit,1000);
#endif
}

#endif /* __SMEWS_H__ */
