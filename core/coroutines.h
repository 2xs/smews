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
#ifndef __COROUTINES_H__
#define __COROUTINES_H__

#include "types.h"
#include "handlers.h"
#include "memory.h"

/* This module provides the coroutines used for dynamic content generation.
 * All coroutines are axecuted in a single shared context.
 * Their context (part of stack used) can be backuped and restored.
 * In-flight segments are managed here: informations about all unacknowledged
 * segments are chained and contain enough information to resend the segment
 * when needed: either the data buffer or the routine context before generation.
 * */

/* Types used for coroutines */
typedef char (cr_func_get)(struct args_t *args);
#ifndef DISABLE_POST
typedef char (cr_func_post_out)(uint8_t content_type,void *post_data);
typedef char (cr_func_post_in)(uint8_t content_type, uint8_t part_number, char *filename, void **post_data);
#endif

/* A context (possibly several per coroutine) */
struct cr_context_t {
#ifdef USE_FRAME_POINTER
	void *sp[2];
#else
	void *sp[1];
#endif
	void *stack;
	unsigned stack_size: 14;
	enum cr_status { cr_ready, cr_active, cr_terminated } status: 2;
};

/* A coroutine, including its current context (one per service) */
struct coroutine_t {
	union func_u {
		cr_func_get *func_get;
#ifndef DISABLE_POST
		cr_func_post_in *func_post_in;
		cr_func_post_out *func_post_out;
#endif
	} func;
	union params_u {
		struct args_t *args;
#ifndef DISABLE_POST
		struct in_t { /* used with dopostin function */
			uint8_t content_type;
			uint8_t part_number; /* number of part with multipart data (0 if one part) */
			char *filename; /* filename of current file */
			void *post_data; /* data flowing between dopostin and dopostout functions */
		} in;
		struct out_t { /* used with dopostout function */
			uint8_t content_type;
			void *post_data; /* data flowing between dopostin and dopostout functions */
		} out;
#endif
	} params;
	struct cr_context_t curr_context;
};

/* Types used for in-flight segments */

/* Possible http headers being used */
/*enum service_header_e {	header_none, header_standard, header_chunks };
*/
/* Information about one in-flight segment (several per service) */
/*struct in_flight_infos_t {
	unsigned char next_outseqno[4]; /* associated sequence number /
	unsigned char checksum[2]; /* segment checksum /
	union if_infos_e { /* either a coroutine context or a data buffer /
		struct cr_context_t *context;
		char *buffer;
	} infos;
	struct in_flight_infos_t *next; /* the next in-flight segment /
	enum service_header_e service_header: 2; /* http header infos /
};*/
struct in_flight_infos_t;

/* Structure used to store information about the service of a generator */
/*
struct generator_service_t {
	unsigned char curr_outseqno[4];
	struct coroutine_t coroutine;
	struct in_flight_infos_t *in_flight_infos;
	enum service_header_e service_header: 2;
	unsigned is_persistent: 1;
};
*/
struct generator_service_t;

#ifndef DISABLE_POST
/* enum with coroutine type */
enum coroutine_type_e { cor_type_get, cor_type_post_in, cor_type_post_out } coroutine_type;
#endif

/* Functions and variables for coroutines */

/* initialize a coroutine structure */
extern void cr_init(struct coroutine_t *coroutine);
/* prepare a coroutine for usage: the context is copied in the shared stack
 * (which information is possibly backuped in the coroutine using it) */
extern struct coroutine_t *cr_prepare(struct coroutine_t *coroutine);
/* run a context or return to main program (if coroutine is NULL) */
extern void cr_run(struct coroutine_t *coroutine
#ifndef DISABLE_POST
		, enum coroutine_type_e type
#endif
		);
/* deallocate the memory used by a coroutine (not including in-flight segments) */
extern void cr_clean(struct coroutine_t *coroutine);

/* select the in-flight segment related to a given sequence number */
extern struct in_flight_infos_t *if_select(struct generator_service_t *service, unsigned char next_outseqno[]);
/* add a new in-flight segment, associate a context to it */
extern struct cr_context_t *context_backup(struct generator_service_t *service, unsigned char next_outseqno[], struct in_flight_infos_t *new_if_infos);
/* restore the context of a given in-flight segment (related to its sequence number) */
extern struct in_flight_infos_t *context_restore(struct generator_service_t *service, unsigned char next_outseqno[]);

#endif /* __COROUTINES_H__ */
#endif
