#include "cal_shared.h"

PERSISTENT_VAR(struct event_t, events[MAX_EVENTS]) = {
};

PERSISTENT_VAR(uint16_t, n_events) = 0;

