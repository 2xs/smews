/*
<generator>
	<handlers doGet="doGet"/>
</generator>
*/

#include "generators.h"
static char doGet(struct args_t *args) {
	out_str("Hello World!");
	return 1;
}
