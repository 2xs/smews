#include "smews.h"

unsigned char smews_loop()
{
	smews_main_loop_step();
}

void smews_first()
{
	smews_init();
}
