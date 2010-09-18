/*
<generator>
	<handlers doGet="doGet"/>
	<properties persistence="volatile"/>
	<args>
		<arg name="val" type="uint16" />
	</args>
</generator>
*/

#include "channels.h"

uint16_t val;

/* triggers the knockknock comet channel */
static char doGet(struct args_t *args) {
	if(args) {
		val = args->val;
		server_push(&knockknock);
		out_str("knock knock: ");
		out_uint(val);
	} else {
		out_str("no args");
	}
	return 1;
}
