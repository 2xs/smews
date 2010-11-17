/*
* Copyright or © or Copr. 2010, Thomas SOETE
* 
* Author e-mail: thomas@soete.org
* Library website : http://mbed.org/users/Alkorin/libraries/SimpleLib/
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

#ifndef __SIMPLELIB_MBED_GLOBALS_H__
#define __SIMPLELIB_MBED_GLOBALS_H__

#include <LPC17xx.h>

/* GLOBALS MACRO */
#define GET_REGISTER8(reg)  *(volatile uint8_t *)(reg)
#define GET_REGISTER16(reg) *(volatile uint16_t *)(reg)
#define GET_REGISTER32(reg) *(volatile uint32_t *)(reg)

#define SET_REGISTER8(reg, val)  *(uint8_t *)(reg)=(val)
#define SET_REGISTER16(reg, val) *(uint16_t *)(reg)=(val)
#define SET_REGISTER32(reg, val) *(uint32_t *)(reg)=(val)

// See 34.3.2.5 p740
#define BIT_BANDING_ADDRESS(reg, bit) (((reg) & 0xF0000000) | (0x02000000) | (((reg) & 0x000FFFFF) << 5) | ((bit) << 2))
#define GET_BIT_ADDRESS(reg, bit) BIT_BANDING_ADDRESS(((uint32_t)&(reg)), (bit))
#define GET_BIT_VALUE(reg, bit) GET_REGISTER32(GET_BIT_ADDRESS((reg), (bit)))
#define SET_BIT_VALUE(reg, bit, value) SET_REGISTER32(GET_BIT_ADDRESS((reg), (bit)), (value))

// Macro tools
#define TOKENPASTE(x, y) x ## y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)

// Extern C
#ifdef __cplusplus
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C
#endif

/** Constants **/
// Peripheral Clock Selection register bit values (Table 42, p57)
#define CCLK4   0U
#define CCLK    1U
#define CCLK2   2U
#define CCLK8   3U

#endif