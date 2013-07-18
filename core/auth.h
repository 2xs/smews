#ifdef HTTP_AUTH

#ifndef __AUTH_H__
#define __AUTH_H__

#include "handlers.h"
#include "types.h"

/* Shortcut to the http 401 handler */
#define http_401_handler apps_httpCodes_401_html_handler

/* Restriction/authentication macro */
#define IS_RESTRICTED(handler) (handler->handler_restriction.realm)
#define IS_AUTHENTICATED(handler) (handler)

/* Http 401 handler */
extern CONST_VAR(struct output_handler_t, apps_httpCodes_401_html_handler);

/* http_401_handler_length:
 *
 * Returns the length of the 401 handler that matches the specified
 * restricted handler, including the dynamically generated http header with
 * correct realm and nonce (if digest auth).
 */
extern uint32_t http_401_handler_length(struct output_handler_t *restricted_handler);

/* http_401_handler_length:
 *
 * Returns the length of the dynamically generated http header for a 401 handler.
 */
extern uint32_t http_401_handler_dynamic_header_length(struct output_handler_t *restricted_handler);
						      
#if HTTP_AUTH == HTTP_AUTH_BASIC

extern CONST_VAR(char, HttpAuthenticateHeader[]);

#define HTTP_AUTHENTICATE_HEADER_LEN 69

#define HTTP_AUTHENTICATE_HEADER_CHK 0x109Eu

#elif HTTP_AUTH == HTTP_AUTH_DIGEST

extern CONST_VAR(char, HttpAuthenticateHeader[]);
extern CONST_VAR(char, HttpAuthenticateHeaderNonce[]);

#define HTTP_AUTHENTICATE_HEADER_LEN 70
#define HTTP_AUTHENTICATE_HEADER_NONCE_LEN 8

#define HTTP_AUTHENTICATE_HEADER_CHK 0x9A92u
#define HTTP_AUTHENTICATE_HEADER_NONCE_CHK 0x6E30u

#endif

#endif
#endif
