/*
<generator>
	<handlers init="init" doGet="doGet"/>
	<properties persistence="volatile" interaction="stream" channel="myStreamedComet"/>
</generator>
*/

#include "generators.h"
#include "timers.h"
#include "channels.h"

/* timer callback function */
static void timer() {
	/* triggers the channel */
	server_push(&myStreamedComet);
}

static char init(void) {
	/* initilize a timer with a period of 1000 milliseconds */
	return set_timer(&timer,1000);
}

static char doGet(struct args_t *args) {
	/* outputs the current time in milliseconds */
	out_uint(TIME_MILLIS);
	return 1;
}
