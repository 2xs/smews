#include <stdio.h>
#include "symbols.h"
#include "../../core/generators.h"
#include "../../core/connections.h"
#include "../../core/timers.h"
#include "../../core/memory.h"


const struct symbols symbols[] = {
	{"get_all_connections",     get_all_connections     },
	{"get_current_remote_ip",   get_current_remote_ip   },
	{"get_local_ip",            get_local_ip            },
#ifndef DISABLE_GP_IP_HANDLER
	{"get_payload_size",        get_payload_size        },
	{"get_protocol",            get_protocol            },
#endif
	{"get_remote_ip",           get_remote_ip           },

#ifndef DISABLE_GP_IP_HANDLER
	{"get_send_code",           get_send_code           },
#endif

#if  !defined(DISABLE_POST) || !defined(DISABLE_GP_IP_HANDLER)
	{"in",                      in         },
#endif
	{"mem_alloc",               mem_alloc  },
        {"mem_free",                mem_free   },
	{"out_c",                   out_c      },
	{"out_str",                 out_str    },
	{"out_uint",                out_uint   },
	{"printf",                  printf},

#ifndef DISABLE_GP_IP_HANDLER
	{"request_packet_out_call", request_packet_out_call },
#endif

#ifndef DISABLE_COMET
	{"server_push",             server_push},
#endif
#ifndef DISABLE_TIMERS
	{"set_timer",               set_timer},
#endif
	{"something_to_send",       something_to_send },
};

const int symbols_nelts = sizeof(symbols)/sizeof(struct symbols);
