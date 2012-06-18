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

#ifndef __GENERATOR_H__
#define __GENERATOR_H__

#include "types.h"
#include "handlers.h"
#include "memory.h"
#include "target.h"
#include "checksum.h"
#include "connections.h"

/* Triggers a Comet channel */
#ifndef DISABLE_COMET
extern char server_push(const struct output_handler_t * /*CONST_VAR*/ push_handler);
#endif
/* Used to output a char, an unisnged int or a string */
char out_c(char c);
extern void out_uint(uint16_t i);
extern void out_str(const char str[]);
#if  !defined(DISABLE_POST) || !defined(DISABLE_GP_IP_HANDLER)
extern short in();
#endif

#endif /* __GENERATOR_H__ */
