/*
* Copyright or © or Copr. 2008, Geoffroy Cogniaux
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

#ifndef PROTO_ERRNO_H
#define PROTO_ERRNO_H

#define _DEBUG

typedef enum {
	SW_OK                       =0x9000,
	SW_WRONG_CLA                =0x6E00,
	SW_WRONG_CONDITION          =0x6985,
	SW_WRONG_DATA               =0x6A80,
	SW_WRONG_INS                =0x6D00,
	SW_WRONG_LE                 =0x6C00,
	SW_WRONG_LEN                =0x6700,
	SW_WRONG_P1P2               =0x6A86,
	SW_WRONG_REFERENCE          =0x6B00,
	//HOT SPECIAL CONTROL SPICES AND OTHER CHILI DEBUG
	SW_CONTROL_OUTPUT           =0x9100,
	SW_CONTROL_DONE             =0x9200,
	SW_CONTROL_ERROR            =0x9300,
	SW_CONTROL_INPUT            =0x9400,
	SW_CONTROL_READ             =0x9500,
	SW_CONTROL_DEBUG            =0x9600
} SW;

#endif /* PROTO_ERRNO_H */

