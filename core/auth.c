#include "auth.h"

#ifdef HTTP_AUTH

#if HTTP_AUTH == HTTP_AUTH_BASIC

CONST_VAR(char, HttpAuthenticateHeader[]) = "\
HTTP/1.1 401 Authorization required\r\nWWW-Authenticate: Basic realm=\
";

#elif HTTP_AUTH == HTTP_AUTH_DIGEST

CONST_VAR(char, HttpAuthenticateHeader[]) = "\
HTTP/1.1 401 Authorization required\r\nWWW-Authenticate: Digest realm=\
";
CONST_VAR(char, HttpAuthenticateHeaderNonce[]) = "\
, nonce=\
";

#endif

uint32_t http_401_handler_dynamic_header_length(struct output_handler_t *restricted_handler)
{
  unsigned const char *char_p = restricted_handler->handler_restriction.realm;
  uint32_t length = 0;

  /* Error checking */
  if (char_p == NULL)
    return -1;

  /* Realm length computation */
  while (*char_p++)
    ++length;

#if HTTP_AUTH == HTTP_AUTH_BASIC  

  /* Remaining size computation */
  length += sizeof(HttpAuthenticateHeader) - 1;
  length += 2; /* Realm must be double quoted */

#elif HTTP_AUTH == HTTP_AUTH_DIGEST

  /* Nonce length computation */
  char_p = http_auth_nonce;
  while (*char_p++)
    ++length;

  /* Remaining size computation */
  length += sizeof(HttpAuthenticateHeader) - 1;
  length += sizeof(HttpAuthenticateHeaderNonce) - 1;
  length += 4; /* Realm and nonce must be double quoted. */

#endif

  return length;
}

uint32_t http_401_handler_length(struct output_handler_t *restricted_handler)
{
    /* Add the size of remaining bytes */
  return http_401_handler_dynamic_header_length (restricted_handler)
    + CONST_UI32(GET_FILE(&http_401_handler).length);
}

#endif
