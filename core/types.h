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

#ifndef __TYPES_H__
#define __TYPES_H__

/*  Allows to define ENDIANNESS as LITTLE_ENDIAN or BIG_ENDIAN */
#ifndef BIG_ENDIAN
	#define BIG_ENDIAN 0
#endif
#ifndef LITTLE_ENDIAN
	#define LITTLE_ENDIAN 1
#endif

/* Include the target dependent target.h file */
#include "target.h"

/* Define NULL is needed */
#ifndef NULL
	#define NULL ((void*)0)
#endif

/* Constant declaration */
#ifndef CONST_VOID_P_VAR
	#define CONST_VOID_P_VAR const void *
#endif
#ifndef CONST_VAR
	#define CONST_VAR(type,name) type const name
#endif

/* Read constant "variables" */
#ifndef CONST_READ_UI8
	#define CONST_READ_UI8(x) (*((uint8_t*)(x)))
#endif
#ifndef CONST_READ_UI16
	#define CONST_READ_UI16(x) (*((uint16_t*)(x)))
#endif
#ifndef CONST_READ_UI32
	#define CONST_READ_UI32(x) (*((uint32_t*)(x)))
#endif
#ifndef CONST_READ_ADDR
	#define CONST_READ_ADDR(x) (*((void**)(x)))
#endif

/* Endianness has to be defined */
#if ENDIANNESS != LITTLE_ENDIAN && ENDIANNESS != BIG_ENDIAN
	#error ENDIANNESS has to be defined in "target.h" as LITTLE_ENDIAN or BIG_ENDIAN
#endif

/* Wn: weight of the n_th byte of a 32 bits integer */
#if ENDIANNESS == LITTLE_ENDIAN
	#define W0 3
	#define W1 2
	#define W2 1
	#define W3 0
#else
	#define W0 0
	#define W1 1
	#define W2 2
	#define W3 3
#endif
/* Sn: weight of the n_th byte of a 16 bits integer */
#if ENDIANNESS == LITTLE_ENDIAN
	#define S0 1
	#define S1 0
#else
	#define S0 0
	#define S1 1
#endif

/* Cast variable as uint16_t or uint32_t */
#define UI16(x) (*((uint16_t*)(x)))
#define UI32(x) (*((uint32_t*)(x)))

#endif /* __TYPES_H__ */
