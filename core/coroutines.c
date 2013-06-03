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
#ifndef DISABLE_COROUTINES

#include "coroutines.h"
#include "output.h"
#include "connections.h"
#include "memory.h"

/* Stack pointer of the main program */
#ifdef USE_FRAME_POINTER
volatile void *main_sp[2];
#else
volatile void *main_sp[1];
#endif

/* Shared stack used for the execution of all coroutines */
static char shared_stack[STACK_SIZE];
/* The coroutine currently using the stack */
static struct coroutine_t *cr_in_stack = NULL;
/* Currently running coroutine. Null when the main program is running. */
static volatile struct coroutine_t *current_cr = NULL;

/* initialize a coroutine structure */
void cr_init(struct coroutine_t *coroutine) {
	coroutine->curr_context.status = cr_ready;
	coroutine->curr_context.stack = NULL;
	coroutine->curr_context.stack_size = 0;
	/* the stack pointer starts at the end of the shared stack */
#ifndef INITIAL_SHARED_STACK_SP
	coroutine->curr_context.sp[0] = shared_stack + STACK_SIZE;
#else
	coroutine->curr_context.sp[0] = INITIAL_SHARED_STACK_SP(shared_stack,STACK_SIZE);
#endif

#ifdef USE_FRAME_POINTER
	coroutine->curr_context.sp[1] = shared_stack + STACK_SIZE;
#endif
}

/* run a context or return to main program (if coroutine is NULL) */
void cr_run(struct coroutine_t *coroutine
#ifndef DISABLE_POST
		, enum coroutine_type_e type
#endif
		) {
	/* push all working registers */
	PUSHREGS;
	/* backup current context stack pointer(s) */
	if(current_cr) {
		BACKUP_CTX(current_cr->curr_context.sp);
	} else {
		BACKUP_CTX(main_sp);
	}
	/* set new current context */
	current_cr = coroutine;
	if(current_cr) {
		RESTORE_CTX(current_cr->curr_context.sp);
		/* test if this is the first time we run this context */
		if(current_cr->curr_context.status == cr_ready) {
			current_cr->curr_context.status = cr_active;
#ifndef DISABLE_POST
			if(type == cor_type_post_out)
				current_cr->func.func_post_out(current_cr->params.out.content_type,current_cr->params.out.post_data);
			else if(type == cor_type_post_in)
			    current_cr->func.func_post_in(current_cr->params.in.content_type,current_cr->params.in.part_number,current_cr->params.in.filename,(void**)&current_cr->params.in.post_data);
			else
#endif
				current_cr->func.func_get(current_cr->params.args);
			current_cr->curr_context.status = cr_terminated;
			current_cr = NULL;
		}
	}
	if(current_cr == NULL) {
		/* restore the main program stack pointer if needed */
		RESTORE_CTX(main_sp);
	}
	/* pop all working registers */
	POPREGS;
	return;
}

/* prepare a coroutine for usage: the context is copied in the shared stack
 * (which information is possibly backuped in the coroutine using it) */
struct coroutine_t *cr_prepare(struct coroutine_t *coroutine) {
	if(cr_in_stack != coroutine) {
		if(cr_in_stack != NULL) { /* is there a coroutine currently using the shared stack? */
			/* backup its context in a freshly allocated buffer of the exact needed size */
			char *sp = cr_in_stack->curr_context.sp[0];
			uint16_t stack_size = shared_stack + STACK_SIZE - sp;
			cr_in_stack->curr_context.stack = mem_alloc(stack_size); /* test NULL: done */
			if(cr_in_stack->curr_context.stack == NULL) {
				return NULL;
			}
			cr_in_stack->curr_context.stack_size = stack_size;
			/* process the copy from (big) shared stack to the new (small) buffer */
			memcpy(cr_in_stack->curr_context.stack, sp, stack_size);
		}
		if(coroutine->curr_context.stack != NULL) { /* does the new coroutine already has an allocated stack? */
			/* restore its context to the (big) shared stack */
			memcpy(coroutine->curr_context.sp[0], coroutine->curr_context.stack, coroutine->curr_context.stack_size);
			/* deallocate the (small) previously used stack */
			mem_free(coroutine->curr_context.stack, coroutine->curr_context.stack_size);
			coroutine->curr_context.stack = NULL;
		}
		/* update the cr_in_stack pointer */
		cr_in_stack = coroutine;
	}
	return cr_in_stack;
}

/* deallocate the memory used by a coroutine (not including in-flight segments) */
void cr_clean(struct coroutine_t *coroutine) {
	if(cr_in_stack == coroutine) {
		cr_in_stack = NULL;
	}
	mem_free(coroutine->curr_context.stack, coroutine->curr_context.stack_size);
}


/* select the in-flight segment related to a given sequence number */
struct in_flight_infos_t *if_select(struct generator_service_t *service, unsigned char next_outseqno[]) {
	struct in_flight_infos_t *if_infos = service->in_flight_infos;
	while(if_infos && UI32(if_infos->next_outseqno) != UI32(next_outseqno)) {
		if_infos = if_infos->next;
	}
	return if_infos;
}

/* add a new in-flight segment, associate a context to it */
struct cr_context_t *context_backup(struct generator_service_t *service, unsigned char next_outseqno[], struct in_flight_infos_t *new_if_infos) {
	char *sp = service->coroutine.curr_context.sp[0];
	uint16_t stack_size = shared_stack + STACK_SIZE - sp;
	char *new_stack = NULL;

	/* try to allocate a context and a stack (exactly fitting the coroutine stack usage) */
	struct cr_context_t *new_context = mem_alloc(sizeof(struct cr_context_t)); /* test NULL: done */
	if(new_context == NULL) {
		return NULL;
	}
	if(stack_size) {
		new_stack = mem_alloc(stack_size); /* test NULL: done */
		if(new_stack == NULL) {
			mem_free(new_context, sizeof(struct cr_context_t));
			return NULL;
		}
	}

	/* process the context copy */
	memcpy(new_context, &service->coroutine.curr_context, sizeof(struct cr_context_t));

	/* process the stack copy */
	new_context->stack_size = stack_size;
	if(new_context->stack_size) {
		new_context->stack = new_stack;
		memcpy(new_stack, sp, stack_size);
	} else {
		new_context->stack = NULL;
	}

	/* update the in-flight segments list */
	UI32(new_if_infos->next_outseqno) = UI32(next_outseqno);
	new_if_infos->infos.context = new_context;
	new_if_infos->next = service->in_flight_infos;

	service->in_flight_infos = new_if_infos;

	return new_context;
}

/* restore the context of a given in-flight segment (related to its sequence number) */
struct in_flight_infos_t *context_restore(struct generator_service_t *service, unsigned char next_outseqno[]) {
	struct in_flight_infos_t *if_infos = if_select(service, next_outseqno);
	if(if_infos) {
		/* copy the (small) context in if-infos in the (big) shared stack */
		memcpy(&service->coroutine.curr_context, if_infos->infos.context, sizeof(struct cr_context_t));
		if(if_infos->infos.context->stack) {
			memcpy(service->coroutine.curr_context.sp[0], if_infos->infos.context->stack, if_infos->infos.context->stack_size);
		}
	}
	return if_infos;
}

#endif
