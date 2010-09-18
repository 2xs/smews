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
	<properties persistence="idempotent"/>
</generator>
*/

#include "cb_shared.h"

static CONST_VAR(unsigned char, fields_names[5][16]) = {
	{"name"},
	{"email"},
	{"phone"},
	{"company"},
	{"address"},
};

static CONST_VAR(unsigned char, str0[]) = "[";
static CONST_VAR(unsigned char, str1[]) = "{";
static CONST_VAR(unsigned char, str2[]) = ":\"";
static CONST_VAR(unsigned char, str3[]) = "\",";
static CONST_VAR(unsigned char, str4[]) = "},";
static CONST_VAR(unsigned char, str5[]) = "]";

static void out_const_str(const unsigned char /*CONST_VAR*/ *str) {
	const unsigned char *c = str;
	unsigned char tmp;
	while((tmp = CONST_READ_UI8(c++))!='\0'){
		out_c(tmp);
	}
}

static char doGet(struct args_t *args) {
	uint16_t i = 0;
	uint16_t n = CONST_UI16(n_contacts);

	out_const_str(str0);
	
	for(i=0; i<n; i++) {
		unsigned char j;
		out_const_str(str1);
		for(j=0; j<5; j++) {
			out_const_str(fields_names[j]);
			out_const_str(str2);
			out_const_str(contacts[i].fields[j]);
			out_const_str(str3);
		}
		out_const_str(str4);
	}

	out_const_str(str5);

	return 1;
}
