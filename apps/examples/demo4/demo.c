/*
<generator>
	<handlers doGet="doGet"/>
	<args>
		<arg name="i1" type="uint8" />
		<arg name="s" type="str" size="6" />
		<arg name="i2" type="uint16" />
	</args>
</generator>
*/

#include "generators.h"

static char doGet(struct args_t *args) {
	if(args) {
		out_str("first int : ");
		out_uint(args->i1);
		out_str("\nstr : ");
		out_str(args->s);
		out_str("\nsecond int : ");
		out_uint(args->i2);
	} else {
		out_str("no args");
	}
	return 1;
}
