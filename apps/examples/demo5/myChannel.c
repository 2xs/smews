/*
<generator>
	<handlers initGet="initGet" doGet="doGet"/>
	<properties persistence="volatile" interaction="alert" channel="knockknock"/>
</generator>
*/

extern uint16_t val;

/* launched when the GET request is received */
static char initGet(struct args_t *args) {
	return 1;
}

/* launched when knockknock is triggered */
static char doGet(struct args_t *args) {
	out_str("somebody knocked: ");
	out_uint(val);
	return 1;
}
