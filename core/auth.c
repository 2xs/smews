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
 */

#include "auth.h"
#include "md5.h"

#ifdef HTTP_AUTH

#include "http_auth_data.h"

#if HTTP_AUTH == HTTP_AUTH_BASIC
CONST_VAR(unsigned char, HttpAuthenticateScheme[]) = "basic";

CONST_VAR(char, HttpAuthenticateHeader[]) = "\
HTTP/1.1 401 Authorization required\r\nWWW-Authenticate: Basic  realm=\
";

#elif HTTP_AUTH == HTTP_AUTH_DIGEST
CONST_VAR(unsigned char, HttpAuthenticateScheme[]) = "digest";

CONST_VAR(char, HttpAuthenticateHeader[]) = "\
HTTP/1.1 401 Authorization required\r\nWWW-Authenticate: Digest realm=\
";

CONST_VAR(char, HttpAuthenticateHeaderNonce[]) = "\
, nonce=\
";

CONST_VAR(char, HttpAuthenticateHeaderStale[]) = "\
, stale=true\
";

struct nonce_state_s nonce_state = {
  .requester = NULL,
  .nonce = "\0"
};

#endif

uint32_t http_401_handler_dynamic_header_length(const struct output_handler_t *restricted_handler
#if HTTP_AUTH == HTTP_AUTH_DIGEST
						, const uint8_t stale
#endif
						)
{
  unsigned const char *char_p = restricted_handler->handler_restriction.realm;
  uint32_t length = 0;

  /* Error checking */
  if (char_p == NULL)
    return -1;

  /* Realm length computation */
  while (*char_p++)
    ++length;

  /* If realm length is odd, we'll add a space after it */
  if (length & 1)
    ++length;

#if HTTP_AUTH == HTTP_AUTH_BASIC  

  /* Remaining size computation */
  length += sizeof (HttpAuthenticateHeader) - 1;
  length += 2; /* Realm must be double quoted */

#elif HTTP_AUTH == HTTP_AUTH_DIGEST

  /* Remaining size computation */
  length += sizeof (HttpAuthenticateHeader) - 1;
  length += sizeof (HttpAuthenticateHeaderNonce) - 1;
  if (stale)
    length += sizeof (HttpAuthenticateHeaderStale) - 1;

  length += 32; /* Nonce is a 32-Hex character string */
  length += 4;  /* Realm and nonce must be double quoted. */

#endif

  return length;
}

uint32_t http_401_handler_length(const struct output_handler_t *restricted_handler
#if HTTP_AUTH == HTTP_AUTH_DIGEST
				 , const uint8_t stale
#endif
				 )
{
  /* Add the size of remaining bytes */
  return http_401_handler_dynamic_header_length (restricted_handler
#if HTTP_AUTH == HTTP_AUTH_DIGEST						 
						 , stale
#endif
						 )
    + CONST_UI32 (GET_FILE (&http_401_handler).length);
}

#if HTTP_AUTH == HTTP_AUTH_BASIC
char http_basic_authenticate(const struct output_handler_t *handler,
			     unsigned const char *credentials)
{
  unsigned const char *current_credentials;
  unsigned const char *given_credentials;
  uint8_t i;

  /* For each valid credentials ... */
  for (i = handler->handler_restriction.credentials_offset;
       i < (handler->handler_restriction.credentials_offset
	    + handler->handler_restriction.credentials_count);
       ++i) {

    current_credentials = http_auth_basic_credentials_data[i];
    given_credentials = credentials;

    /* String comparison loop */
    while (*given_credentials == *current_credentials) {
      /* If we reach end of both string simultaneously, they match. */
      if (*given_credentials == '\0' && *current_credentials == '\0')
	return HTTP_AUTHENTICATE_SUCCESS;

      ++given_credentials;
      ++current_credentials;
    }    
  }

  return HTTP_AUTHENTICATE_FAILURE;
}

#elif HTTP_AUTH == HTTP_AUTH_DIGEST
char http_digest_authenticate(const struct connection *requester,
			      const struct output_handler_t *handler,
			      const struct auth_data_s *auth_data)
{
  credentials_digest_t *digest_infos = NULL;
  uint8_t i, j;

  /* Nonce validity check */
  for (i = 0; i < NONCE_LEN; ++i)
    if (auth_data->nonce[i] != nonce_state.nonce[i])
      return HTTP_AUTHENTICATE_STALE;

  /* User validity check */
  for (i = handler->handler_restriction.credentials_offset;
       i < (handler->handler_restriction.credentials_offset
	    + handler->handler_restriction.credentials_count);
       ++i) {

    if (http_auth_digest_credentials_table[i] != auth_data->username_blob)
      continue;

    /* Once username has been validated, get the matching credentials */
    for (j = 0; j < USER_DIGEST_COUNT; ++j) {
      if (http_auth_digest_credentials_data[j].username_blob == auth_data->username_blob
	  &&
	  http_auth_digest_credentials_data[j].realm == handler->handler_restriction.realm) {
	digest_infos = (credentials_digest_t *) &http_auth_digest_credentials_data[j];
	break;
      }
    }
    break;
  }

  /* No digest informations: invalid username */
  if (digest_infos == NULL)
    return HTTP_AUTHENTICATE_FAILURE;

  /* Compute the valid md5 digest :
   * md5(user_digest:nonce:resource_digest) */
  md5_init();
  md5_append((const md5_byte_t *) digest_infos->user_digest, MD5_DIGEST_LEN);
  md5_append((const md5_byte_t *) ":", 1);
  md5_append((const md5_byte_t *) nonce_state.nonce, NONCE_LEN);
  md5_append((const md5_byte_t *) ":", 1);
  md5_append((const md5_byte_t *) handler->handler_restriction.resource_digest,
	     MD5_DIGEST_LEN);
  md5_end();

  /* Compare given credentials with computed credentials */
  for (i = 0; i < MD5_DIGEST_LEN; ++i) {
    /* WARNING: md5_digest is a 16-byte table, whereas credentials is a classic
     *          C character string !
     *		Don't try something like "credentials[i] == md5_digest[i]" */
    if (auth_data->response[i] != MD5_DIGEST_GET_NTH_HEXC(i))
      return HTTP_AUTHENTICATE_FAILURE;
  }

  return HTTP_AUTHENTICATE_SUCCESS;
}

void http_auth_gen_nonce()
{
  static uint32_t x = 123456789; /* XORSHIFT data */
  static uint32_t y = 362436069; /* XORSHIFT data */
  static uint32_t z = 521288629; /* XORSHIFT data */
  static uint32_t w = 88675123;  /* XORSHIFT data */
  uint32_t tmp;
  uint8_t i;

  for (i = 0; i < NONCE_LEN; i += 8) {
    /* Compute xorshift */
    tmp = x ^ (x << 1);
    x = y;
    y = z;
    z = w;
    w = w ^ (w >> 19) ^ (tmp ^ (tmp >> 8));

    /* Fill nonce with hex character */
    nonce_state.nonce[i]     = hex_char[(w & 0xf0000000) >> 28];
    nonce_state.nonce[i + 1] = hex_char[(w & 0x0f000000) >> 24];
    nonce_state.nonce[i + 2] = hex_char[(w & 0x00f00000) >> 20];
    nonce_state.nonce[i + 3] = hex_char[(w & 0x000f0000) >> 16];
    nonce_state.nonce[i + 4] = hex_char[(w & 0x0000f000) >> 12];
    nonce_state.nonce[i + 5] = hex_char[(w & 0x00000f00) >> 8];
    nonce_state.nonce[i + 6] = hex_char[(w & 0x000000f0) >> 4];
    nonce_state.nonce[i + 7] = hex_char[(w & 0x0000000f)];
  }
}

#endif
#endif
