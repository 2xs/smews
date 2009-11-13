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

#ifndef __HANDLERS_H__
#define __HANDLERS_H__

#include "types.h"

/** Dynamic contents, static contents, TCP control **/

/* Args type */
struct args_t;
/* Generator initialization function */
typedef char (init_service_func_t)(void);
/* Generator contents generation functions */
typedef char (initget_service_func_t)(struct args_t *);
typedef char (doget_service_func_t)(struct args_t *);

/* Generator contents: init and rnu functions */
struct dynamic_contents {
	init_service_func_t *init;
	initget_service_func_t *initget;
	doget_service_func_t *doget;
	enum prop_e { prop_persistent, prop_idempotent, prop_volatile } prop;
};

/* Static file contents, including pre-calculated partiel checksums */
struct static_contents {
	uint32_t length;
	CONST_VOID_P_VAR chk;
	CONST_VOID_P_VAR data;
};

/* TCP control packet */
struct tcp_control {
	unsigned char length;
	unsigned char flags;
};

/* External refs to TCP control packets */
extern CONST_VAR(struct output_handler_t, ref_synack);
extern CONST_VAR(struct output_handler_t, ref_ack);
extern CONST_VAR(struct output_handler_t, ref_finack);
extern CONST_VAR(struct output_handler_t, ref_rst);


/** URLs arguments **/

#ifndef DISABLE_ARGS
/* Argument type */
enum arg_type_e {
	arg_str, arg_ui8, arg_ui16
};

/* Argument reference */
struct arg_ref_t {
	enum arg_type_e arg_type;
	unsigned char arg_size;
	unsigned char arg_offset;
};
#endif


/** Output handlers **/

/* handler structure to be served */
struct output_handler_t {
	enum handler_type_e {type_control, type_file, type_generator} handler_type;
	unsigned char handler_comet;
	unsigned char handler_stream;
	union handler_contents_u {
		/* this synthethise the information needed to generate the next packet in output.c */
		const struct tcp_control control;
		const struct static_contents file;
		const struct dynamic_contents service;
	} handler_contents;
#ifndef DISABLE_ARGS
	struct handler_args_t {
		unsigned const char * /*CONST_VAR*/ args_tree;
		const struct arg_ref_t * /*CONST_VAR*/ args_index;
		uint16_t args_size;
	} handler_args;
#endif
};

/* Macros for accessing output_handler structs */
#define GET_CONTROL(r) ((r)->handler_contents.control)
#define GET_FILE(r) ((r)->handler_contents.file)
#define GET_SERVICE(r) ((r)->handler_contents.service)
#define GET_FLAGS(r) (CONST_UI8((r)->handler_type) == type_control ? CONST_UI8(GET_CONTROL(r).flags) : (TCP_ACK | TCP_PSH))

/* Global output_handler table */
extern CONST_VAR(const struct output_handler_t *, files_index[]);

/* URL tree: parsing an URL to retrieve an output_handler */
extern CONST_VAR(unsigned char, urls_tree[]);

#endif /* __HANDLERS_H__ */
