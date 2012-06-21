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

#ifndef __RECEIVE_H__
#define __RECEIVE_H__

#include "types.h"

/* Used to dump the runtime stack */
#ifdef STACK_DUMP
	#define STACK_DUMP_SIZE 1024
	#define STACK_MAGIC 0x27
	extern int16_t stack_i;
	extern unsigned char *stack_base;
#endif

/* Receive and process a single packet. Returns 1 if a packet has been successfully processed, 0 else. */
extern char smews_receive(void);

#ifndef IPV6
#define IP_HEADER_SIZE 20
#else
#define IP_HEADER_SIZE 40
#endif

#define TCP_HEADER_SIZE 20


#endif /* __RECEIVE_H__ */
