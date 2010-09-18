/*
<generator>
	<handlers doGet="doGet"/>
	<properties persistence="volatile"/>
</generator>
*/

/* generator possible properties are persistent (by default), idempotent and volatile */
static char doGet(struct args_t *args) {
	out_str("Volatile Hello World ! ");
	return 1;
}
