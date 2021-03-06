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
	<properties persistence="volatile" interaction="alert" channel="cometRand"/>
</generator>
*/

#include "generators.h"
#include "timers.h"
#include "channels.h"
#include <math.h>

#define MAX_RAND 50

static unsigned char initialized;
static char randInterval = 0;
static char currInterval = 0;
static uint16_t nPub = 0;
static uint32_t toPublish = 0;

static void timer() {
	if(initialized && --randInterval <= 0) {
		nPub++;
		toPublish = TIME_MILLIS;
		currInterval = randInterval = 1 + ((unsigned char)rand())%MAX_RAND;
		server_push(&cometRand);
	}
}

static char init(void) {
	return set_timer(&timer,1000);
}

static char initGet(struct args_t *args) {
	if(!initialized) {
		initialized = 1;
		srand(TIME_MILLIS);
		currInterval = randInterval = 1 + ((unsigned char)rand())%MAX_RAND;
	}
	return 1;
}

static char doGet(struct args_t *args) {
	out_uint(toPublish);
	out_c('\n');
	out_uint(nPub);
	out_c('\n');
	out_uint(currInterval);
	return 2;
}
