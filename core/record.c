
#include "record.h"
#include "types.h"
#include "tls.h"



/* parse a TLS Record Header; return number of bytes of record data */
/* type can be HANDSHAKE, APP or CCS . TODO : add ALRT */
