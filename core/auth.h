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

#ifndef __AUTH_H__
#define __AUTH_H__

#define HTTP_AUTH_BASIC    0
#define HTTP_AUTH_DIGEST   1

#include "handlers.h"
#include "connections.h"
#include "types.h"

/* Readability++ macros */
#define IS_RESTRICTED(handler)       (handler->handler_restriction.realm)

#define HTTP_AUTHENTICATE_SUCCESS    1
#define HTTP_AUTHENTICATE_FAILURE    0
#define NO_OFFSET (127)  /* Useful for parsing nonce/response in input.c */

/* Shortcut to the http 401 handler */
#define http_401_handler apps_httpCodes_401_html_handler

struct auth_data_s {
  uint8_t scheme_parsed: 1;        /* Flag set on 1 if scheme has been parsed */
  uint8_t parsing_offset: 7;              /* Offset into fields while parsing */
#if HTTP_AUTH == HTTP_AUTH_BASIC
  unsigned char *credentials;          /* Dynamically allocated storage for 
					* basic credentials: b64(user:passwd) */
#elif HTTP_AUTH == HTTP_AUTH_DIGEST
  uint8_t username_blob;                  /* Username identifier in blob tree */
  unsigned char response[32];       /* Response digest as given by the client */
  unsigned char nonce[32];      /* Nonce used by the client to compute digest */
#endif
};

/* Http 401 handler */
extern CONST_VAR(struct output_handler_t, apps_httpCodes_401_html_handler);

/* Store the character string representation of the authentication scheme */
extern CONST_VAR(unsigned char, HttpAuthenticateScheme[]);
/* Store the character string representation of the http authenticate header */
extern CONST_VAR(char, HttpAuthenticateHeader[]);

/* Returns the length of the 401 handler that matches the specified
 * restricted handler, including the dynamically generated http header with
 * correct realm and nonce (if digest auth). */
extern uint32_t http_401_handler_length(const struct output_handler_t *restricted_handler
#if HTTP_AUTH == HTTP_AUTH_DIGEST
					, const uint8_t stale
#endif
					);

/* Returns the length of the dynamically generated http header for a 401 handler. */
extern uint32_t http_401_handler_dynamic_header_length(const struct output_handler_t *restricted_handler
#if HTTP_AUTH == HTTP_AUTH_DIGEST
						       , const uint8_t stale
#endif
						       );

/*************************************************************************/
/********************* AUTHENTICATION KIND SPECIFICS *********************/
/*************************************************************************/

/*********************** BASIC AUTHENTICATION DATA ***********************/
#if HTTP_AUTH == HTTP_AUTH_BASIC

#define HTTP_AUTHENTICATE_HEADER_LEN 68
#define HTTP_AUTHENTICATE_HEADER_CHK 0x428Cu

/* Returns 1 if given credentials are valid for the requested handler, 0 if not. */
extern char http_basic_authenticate(const struct output_handler_t *handler,
				    unsigned const char *credentials);

/*********************** DIGEST AUTHENTICATION DATA **********************/
#elif HTTP_AUTH == HTTP_AUTH_DIGEST

#define HTTP_AUTHENTICATE_STALE      -1

#define HTTP_AUTHENTICATE_HEADER_LEN 68
#define HTTP_AUTHENTICATE_HEADER_CHK 0x9A92u

extern CONST_VAR(char, HttpAuthenticateHeaderNonce[]);
extern CONST_VAR(char, HttpAuthenticateHeaderStale[]);

#define HTTP_AUTHENTICATE_HEADER_NONCE_LEN 8
#define HTTP_AUTHENTICATE_HEADER_NONCE_CHK 0x6E30u

#define HTTP_AUTHENTICATE_HEADER_STALE_LEN 12
#define HTTP_AUTHENTICATE_HEADER_STALE_CHK 0x5016u

#define NONCE_LEN 32

/* Digest authentication data structure */
typedef struct credentials_digest_s {
  const uint8_t username_blob;     /* Corresponding blob into users blob tree */
  const unsigned char *realm;       /* Realm for which this credentials stand */
  const unsigned char user_digest[33];       /* Digest for this user & realm. */
} credentials_digest_t;

/* Nonce control structure */
extern struct nonce_state_s {
  struct connection *requester;         /* Last connection requesting a nonce */
  unsigned char nonce[NONCE_LEN];                     /* Last nonce generated */
} nonce_state;

/* Returns 1 if given credentials are valid for the requested handler, 0 if not.
 * Returns -1 if nonce is not valid (response will contain "stale" in WWW-Auth) */
extern char http_digest_authenticate(const struct connection *requester,
				     const struct output_handler_t *handler,
				     const struct auth_data_s *auth_data);

/* Generate a new nonce based on a xorshift randomizer. */
extern void http_auth_gen_nonce();

#endif
/**************************** END OF SPECIFICS ***************************/

#endif

