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

#ifndef __CHECKSUM_H__
#define __CHECKSUM_H__

#include "types.h"

/* Sums two checksums, taking in account the carry. To be used into other macros */
#define CHK_SUM(a,b) ((((a)+(b)) & 0x0000ffff) + ((((a)+(b)) & 0xffff0000) >> 16))

/* Current checksum */
extern unsigned char current_checksum[2];

#ifndef INLINE_CHECKSUM
/* Checksumming functions */
extern void checksum_init(void);
extern void checksum_set(uint16_t val);
extern void checksum_end(void);
extern void checksum_add(unsigned char x);
/* Te be used only with an even alignment */
void checksum_add16(const uint16_t x);
/* Te be used only with an even alignment */
void checksum_add32(const unsigned char x[]);
#else
#include "checksum.inl"
#endif


#endif /* __CHECKSUM_H__ */
