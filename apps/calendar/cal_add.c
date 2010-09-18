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
	<handlers doGet="doGet"/>
	<properties persistence="persistent"/>
	<args>
		<arg name="id" type="uint8" />
		<arg name="field" type="str" size="31" />
	</args>
</generator>
*/

#include "cal_shared.h"

static char doGet(struct args_t *args) {
	if(args) {
		unsigned char id = args->id;
		if(id < MAX_EVENTS) {
			uint16_t n = CONST_UI16(n_events);
			char len = 0;
			while(args->field[len]) {
				len++;
			}
			if(n <= id) {
				n = id + 1;
				CONST_WRITE_NBYTES((void*)&n_events,(void*)&n,2);
			}
			CONST_WRITE_NBYTES((void*)events[id].message,(void*)args->field,len+1);
			if(!len) {
				unsigned char i;
				len = -1;
				for(i = 0; i<n ; i++) {
					if(CONST_UI8(events[i].message[0])) {
						len = i;
					}
				}
				n = len + 1;
				CONST_WRITE_NBYTES((void*)&n_events,(void*)&n,2);
			}
		}
	}
	return 1;
}
