/*
* Copyright or © or Copr. 2008, Simon Duquennoy
* 
* Author e-mail: simon.duquennoy@lifl.fr
* 
* This software is a computer program whose purpose is to design an
* efficient Web server for very-constrained embedded system.
* 
* This software is governed by the CeCILL license under French law and
* abiding by the rules of distribution of free software.  You can  use, 
* modify and/ or redistribute the software under the terms of the CeCILL
* license as circulated by CEA, CNRS and INRIA at the following URL
* "http://www.cecill.info". 
* 
* As a counterpart to the access to the source code and  rights to copy,
* modify and redistribute granted by the license, users are provided only
* with a limited warranty  and the software's author,  the holder of the
* economic rights,  and the successive licensors  have only  limited
* liability. 
* 
* In this respect, the user's attention is drawn to the risks associated
* with loading,  using,  modifying and/or developing or reproducing the
* software by the user in light of its specific status of free software,
* that may mean  that it is complicated to manipulate,  and  that  also
* therefore means  that it is reserved for developers  and  experienced
* professionals having in-depth computer knowledge. Users are therefore
* encouraged to load and test the software's suitability as regards their
* requirements in conditions enabling the security of their systems and/or 
* data to be ensured and,  more generally, to use and operate it in the 
* same conditions as regards security. 
* 
* The fact that you are presently reading this means that you have had
* knowledge of the CeCILL license and that you accept its terms.
*/

/*
<generator>
	<handlers init="init" initGet="initGet" doGet="doGet"/>
	<properties persitence="volatile" channel="cpuChannel"/>
	<args>
		<arg name="thres" type="uint16" />
	</args>
</generator>
*/

#include <stdio.h>

#include "generators.h"
#include "timers.h"
#include "channels.h"

static uint16_t glob_load;
static int last_sum;
static int last_idle;
static int threshold = 512;

static void timer() {
	char tmp[32];
	int i1,i2,i3,i5,i6,i7,i8,idle;
	int sum;
	float ratio;
	FILE *in = fopen("/proc/stat","r");
	fscanf(in, "%32s %d %d %d %d %d %d %d %d",tmp, &i1, &i2, &i3, &idle, &i5, &i6, &i7, &i8);
	fclose(in);
	sum = idle + i1 + i2 + i3 + i5 + i6 + i7 + i8;
	if(sum != last_sum) {
		ratio = 1024 * (1 - ((idle - last_idle) / (float)(sum - last_sum)));
		last_sum = sum;
		last_idle = idle;
		if(ratio > threshold) {
			glob_load = (uint16_t)ratio;
			server_push(&cpuChannel);
		}
	}
}

static char init(void) {
	return set_timer(&timer,500);
}

static char initGet(struct args_t *args) {
	if(args)
		threshold = args->thres;
	return 1;
}

static char doGet(struct args_t *args) {
	out_uint(glob_load);
	return 1;
}
