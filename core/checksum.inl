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

#ifdef INLINE_CHECKSUM
#define CHECKSUM_CALL static inline
/* current checksum carry */
extern unsigned char checksum_carry;
/* index of the next checksum byte to compute (0/1) */
extern unsigned char checksum_flip;
#else
#define CHECKSUM_CALL
#endif

/*-----------------------------------------------------------------------------------*/
CHECKSUM_CALL void checksum_init() {
	current_checksum[S0] = 0;
	current_checksum[S1] = 0;
	checksum_carry = 0;
	checksum_flip = S0;
}

/*-----------------------------------------------------------------------------------*/
CHECKSUM_CALL void checksum_add(unsigned char val) {
	uint16_t tmp_sum;
	tmp_sum = current_checksum[checksum_flip] + val + checksum_carry;
	current_checksum[checksum_flip] = tmp_sum;
	checksum_carry = tmp_sum >> 8;
	checksum_flip ^= 0x01;	
}

/* Te be used only with an even alignment */
/*-----------------------------------------------------------------------------------*/
CHECKSUM_CALL void checksum_add16(const uint16_t val) {
	uint16_t tmp_sum;
	
	tmp_sum = current_checksum[S0] + (val >> 8) + checksum_carry;
	current_checksum[S0] = tmp_sum;
	checksum_carry = tmp_sum >> 8;

	tmp_sum = current_checksum[S1] + (val & 0xff) + checksum_carry;
	current_checksum[S1] = tmp_sum;
	checksum_carry = tmp_sum >> 8;
}

/* Te be used only with an even alignment */
/*-----------------------------------------------------------------------------------*/
CHECKSUM_CALL void checksum_add32(const unsigned char val[]) {
	uint16_t tmp_sum;
	
	tmp_sum = current_checksum[S0] + val[W0] + val[W2] + checksum_carry;
	current_checksum[S0] = tmp_sum;
	checksum_carry = tmp_sum >> 8;

	tmp_sum = current_checksum[S1] + val[W1] + val[W3] + checksum_carry;
	current_checksum[S1] = tmp_sum;
	checksum_carry = tmp_sum >> 8;
}

/*-----------------------------------------------------------------------------------*/
CHECKSUM_CALL void checksum_end() {	
	while(checksum_carry)
		checksum_add(0);	
}
