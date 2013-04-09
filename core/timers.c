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

#ifndef DISABLE_TIMERS

#include "timers.h"
#include "output.h"
#include "connections.h"

/* Shared global timer list */
static struct timer_t *all_timers;
/* Used to log connections transmission time */
unsigned char last_transmission_time;

/*-----------------------------------------------------------------------------------*/
unsigned char set_timer(timer_func_t callback,uint16_t period_millis) {
	struct timer_t *new_timer = mem_alloc(sizeof(struct timer_t)); /* test NULL: done */
	if(new_timer == NULL) {
		return 0;
	} else {
		new_timer->next = all_timers;
		all_timers = new_timer;
		new_timer->callback = callback;
		new_timer->period_millis = period_millis;
		new_timer->last_time_millis = TIME_MILLIS;
		return 1;
	}
}

/*-----------------------------------------------------------------------------------*/
void smews_timers() {
	struct timer_t *curr_timer = all_timers;
	uint32_t curr_time_millis;
#ifdef SMEWS_WAITING
	SMEWS_WAITING;
#endif
	curr_time_millis = TIME_MILLIS;
	while(curr_timer) {
		while(((uint16_t)(curr_time_millis - curr_timer->last_time_millis)) > curr_timer->period_millis) {
			curr_timer->last_time_millis += (uint32_t)curr_timer->period_millis;
			curr_timer->callback();
		}
		curr_timer = curr_timer->next;
	}
}

#endif
