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
 * Author e-mail: christophe.bacara@etudiant.univ-lille1.fr
 *
 * Original software written by L. Peter Deutsch, for Aladdin Enterprises
 * Improvements and optimizations for embedded target by Christophe Bacara
 */

#include "md5.h"

#define T1 T[0]
#define T2 T[1]
#define T3 T[2]
#define T4 T[3]
#define T5 T[4]
#define T6 T[5]
#define T7 T[6]
#define T8 T[7]
#define T9 T[8]
#define T10 T[9]
#define T11 T[10]
#define T12 T[11]
#define T13 T[12]
#define T14 T[13]
#define T15 T[14]
#define T16 T[15]
#define T17 T[16]
#define T18 T[17]
#define T19 T[18]
#define T20 T[19]
#define T21 T[20]
#define T22 T[21]
#define T23 T[22]
#define T24 T[23]
#define T25 T[24]
#define T26 T[25]
#define T27 T[26]
#define T28 T[27]
#define T29 T[28]
#define T30 T[29]
#define T31 T[30]
#define T32 T[31]
#define T33 T[32]
#define T34 T[33]
#define T35 T[34]
#define T36 T[35]
#define T37 T[36]
#define T38 T[37]
#define T39 T[38]
#define T40 T[39]
#define T41 T[40]
#define T42 T[41]
#define T43 T[42]
#define T44 T[43]
#define T45 T[44]
#define T46 T[45]
#define T47 T[46]
#define T48 T[47]
#define T49 T[48]
#define T50 T[49]
#define T51 T[50]
#define T52 T[51]
#define T53 T[52]
#define T54 T[53]
#define T55 T[54]
#define T56 T[55]
#define T57 T[56]
#define T58 T[57]
#define T59 T[58]
#define T60 T[59]
#define T61 T[60]
#define T62 T[61]
#define T63 T[62]
#define T64 T[63]

static const uint32_t T[64] = {
  0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
  0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
  0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
  0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
  0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
  0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
  0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
  0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
  0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
  0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
  0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
  0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
  0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
  0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
  0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
  0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

/* Define the state of the MD5 Algorithm. */
static struct {
    md5_word_t count[2];	/* message length in bits, lsw first */
    md5_word_t abcd[4];		/* digest buffer */
    md5_byte_t buf[64];		/* accumulate block */
} md5_state;

/* Store the final MD5 digest */
md5_byte_t md5_digest[16];

static void md5_process(const md5_byte_t *data /* [64] */)
{
  md5_word_t /* Temporary word a,b,c,d */
    a = md5_state.abcd[0], b = md5_state.abcd[1],
    c = md5_state.abcd[2], d = md5_state.abcd[3];
  md5_word_t t;
  md5_word_t X[16];
  unsigned char i;

#if ENDIANNESS == LITTLE_ENDIAN
  memcopy(X, data, 64);
#else
  for (i = 0; i < 16; ++i, data += 4)
    X[i] = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
#endif

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

  /* Round 1. */
  /* Let [abcd k s i] denote the operation */
  /*    a = b + ((a + F(b,c,d) + X[k] + T[i]) <<< s). */
#define F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define SET(a, b, c, d, k, s, Ti)		\
  t = a + F(b,c,d) + X[k] + Ti;			\
  a = ROTATE_LEFT(t, s) + b
  /* Do the following 16 operations. */
  SET(a, b, c, d,  0,  7,  T1);
  SET(d, a, b, c,  1, 12,  T2);
  SET(c, d, a, b,  2, 17,  T3);
  SET(b, c, d, a,  3, 22,  T4);
  SET(a, b, c, d,  4,  7,  T5);
  SET(d, a, b, c,  5, 12,  T6);
  SET(c, d, a, b,  6, 17,  T7);
  SET(b, c, d, a,  7, 22,  T8);
  SET(a, b, c, d,  8,  7,  T9);
  SET(d, a, b, c,  9, 12, T10);
  SET(c, d, a, b, 10, 17, T11);
  SET(b, c, d, a, 11, 22, T12);
  SET(a, b, c, d, 12,  7, T13);
  SET(d, a, b, c, 13, 12, T14);
  SET(c, d, a, b, 14, 17, T15);
  SET(b, c, d, a, 15, 22, T16);
#undef SET

  /* Round 2. */
  /* Let [abcd k s i] denote the operation */
  /*    a = b + ((a + G(b,c,d) + X[k] + T[i]) <<< s). */
#define G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define SET(a, b, c, d, k, s, Ti)		\
  t = a + G(b,c,d) + X[k] + Ti;			\
  a = ROTATE_LEFT(t, s) + b
  /* Do the following 16 operations. */
  SET(a, b, c, d,  1,  5, T17);
  SET(d, a, b, c,  6,  9, T18);
  SET(c, d, a, b, 11, 14, T19);
  SET(b, c, d, a,  0, 20, T20);
  SET(a, b, c, d,  5,  5, T21);
  SET(d, a, b, c, 10,  9, T22);
  SET(c, d, a, b, 15, 14, T23);
  SET(b, c, d, a,  4, 20, T24);
  SET(a, b, c, d,  9,  5, T25);
  SET(d, a, b, c, 14,  9, T26);
  SET(c, d, a, b,  3, 14, T27);
  SET(b, c, d, a,  8, 20, T28);
  SET(a, b, c, d, 13,  5, T29);
  SET(d, a, b, c,  2,  9, T30);
  SET(c, d, a, b,  7, 14, T31);
  SET(b, c, d, a, 12, 20, T32);
#undef SET

  /* Round 3. */
  /* Let [abcd k s t] denote the operation */
  /*    a = b + ((a + H(b,c,d) + X[k] + T[i]) <<< s). */
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define SET(a, b, c, d, k, s, Ti)		\
  t = a + H(b,c,d) + X[k] + Ti;			\
  a = ROTATE_LEFT(t, s) + b
  /* Do the following 16 operations. */
  SET(a, b, c, d,  5,  4, T33);
  SET(d, a, b, c,  8, 11, T34);
  SET(c, d, a, b, 11, 16, T35);
  SET(b, c, d, a, 14, 23, T36);
  SET(a, b, c, d,  1,  4, T37);
  SET(d, a, b, c,  4, 11, T38);
  SET(c, d, a, b,  7, 16, T39);
  SET(b, c, d, a, 10, 23, T40);
  SET(a, b, c, d, 13,  4, T41);
  SET(d, a, b, c,  0, 11, T42);
  SET(c, d, a, b,  3, 16, T43);
  SET(b, c, d, a,  6, 23, T44);
  SET(a, b, c, d,  9,  4, T45);
  SET(d, a, b, c, 12, 11, T46);
  SET(c, d, a, b, 15, 16, T47);
  SET(b, c, d, a,  2, 23, T48);
#undef SET

  /* Round 4. */
  /* Let [abcd k s t] denote the operation */
  /*    a = b + ((a + I(b,c,d) + X[k] + T[i]) <<< s). */
#define I(x, y, z) ((y) ^ ((x) | ~(z)))
#define SET(a, b, c, d, k, s, Ti)		\
  t = a + I(b,c,d) + X[k] + Ti;			\
  a = ROTATE_LEFT(t, s) + b
  /* Do the following 16 operations. */
  SET(a, b, c, d,  0,  6, T49);
  SET(d, a, b, c,  7, 10, T50);
  SET(c, d, a, b, 14, 15, T51);
  SET(b, c, d, a,  5, 21, T52);
  SET(a, b, c, d, 12,  6, T53);
  SET(d, a, b, c,  3, 10, T54);
  SET(c, d, a, b, 10, 15, T55);
  SET(b, c, d, a,  1, 21, T56);
  SET(a, b, c, d,  8,  6, T57);
  SET(d, a, b, c, 15, 10, T58);
  SET(c, d, a, b,  6, 15, T59);
  SET(b, c, d, a, 13, 21, T60);
  SET(a, b, c, d,  4,  6, T61);
  SET(d, a, b, c, 11, 10, T62);
  SET(c, d, a, b,  2, 15, T63);
  SET(b, c, d, a,  9, 21, T64);
#undef SET

  /* Then perform the following additions. (That is increment each */
  /* of the four registers by the value it had before this block */
  /* was started.) */
  md5_state.abcd[0] += a;
  md5_state.abcd[1] += b;
  md5_state.abcd[2] += c;
  md5_state.abcd[3] += d;
}

void md5_init()
{
  md5_state.count[0] = 0;
  md5_state.count[1] = 0;
  md5_state.abcd[0] = 0x67452301;
  md5_state.abcd[1] = 0xefcdab89;
  md5_state.abcd[2] = 0x98badcfe;
  md5_state.abcd[3] = 0x10325476;
}

void md5_append(const md5_byte_t *data, uint32_t nbytes)
{
  uint32_t offset = (md5_state.count[0] >> 3) & 63;
  md5_word_t nbits = (md5_word_t)(nbytes << 3);

  if (nbytes <= 0)
    return;

  /* Update the message length. */
  md5_state.count[1] += nbytes >> 29;
  md5_state.count[0] += nbits;
  if (md5_state.count[0] < nbits)
    md5_state.count[1]++;

  /* Process an initial partial block. */
  if (offset) {
    uint32_t copy = (offset + nbytes > 64 ? 64 - offset : nbytes);

    memcopy(md5_state.buf + offset, data, copy);
    if (offset + copy < 64)
      return;
    data += copy;
    nbytes -= copy;
    md5_process(md5_state.buf);
  }

  /* Process full blocks. */
  for (; nbytes >= 64; data += 64, nbytes -= 64)
    md5_process(data);

  /* Process a final partial block. */
  if (nbytes)
    memcopy(md5_state.buf, data, nbytes);
}

void md5_end()
{
  static const md5_byte_t pad[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };
  md5_byte_t data[8];
  int i;

  /* Save the length before padding. */
  for (i = 0; i < 8; ++i)
    data[i] = (md5_byte_t)(md5_state.count[i >> 2] >> ((i & 3) << 3));
  /* Pad to 56 bytes mod 64. */
  md5_append(pad, ((55 - (md5_state.count[0] >> 3)) & 63) + 1);
  /* Append the length. */
  md5_append(data, 8);

  for (i = 0; i < 16; ++i)
    md5_digest[i] = (md5_byte_t)(md5_state.abcd[i >> 2] >> ((i & 3) << 3));
}

/* Useful for improved version of processing */
/* static const uint8_t R[64] = { */
/*   7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, */
/*   5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, */
/*   4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, */
/*   6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21 */
/* }; */
