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

/* 
 * Author: Christophe Bacara
 * `-Mail: christophe.bacara@etudiant.univ-lille1.fr
 *
 * Original software written by L. Peter Deutsch, for Aladdin Enterprises
 * Few improvements for embedded target by Christophe Bacara
 */

#ifndef __MD5_H__
#define __MD5_H__

#include "utils.h"

#include <stdint.h>
#include <stdio.h>

#define MD5_DIGEST_LEN 32

/* As digest is stored on 16 bytes, useful to get the nth character
 * of the hexadecimal representation of the digest */
#define MD5_DIGEST_GET_NTH_HEXC(i) \
  (hex_char[((i & 1) ? (md5_digest[i >> 1] & 0x0f) : ((md5_digest[i >> 1] & 0xf0) >> 4))])

/* Internal type definitions */
typedef unsigned char md5_byte_t;    /* 8-bit byte */
typedef uint32_t md5_word_t;         /* 32-bit word */

/* Digest storage */
extern md5_byte_t md5_digest[16];

/* Initialize the algorithm. */
extern void md5_init();
/* Append a string to the message. */
extern void md5_append(const md5_byte_t *data, uint32_t nbytes);
/* Finish the message and return the digest. */
extern void md5_end();

#endif
