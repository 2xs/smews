#include "target.h"

#define MAX_EVENTS 32
#define FIELD_LENGTH 32

struct event_t {
	unsigned char message[FIELD_LENGTH];
};

extern PERSISTENT_VAR(struct event_t, events[MAX_EVENTS]);
extern PERSISTENT_VAR(uint16_t, n_events);

