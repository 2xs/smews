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

/* Parts of code from SOSSE: */

/*
* Simple Operating system for Smart cards
* Copyright (C) 2002  Matthias Bruestle <m@mbsks.franken.de>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef PPSCS_CONFIG_H
#define PPSCS_CONFIG_H


#if defined(__AVR_AT90S8515__)
//! This is a little endian architecture.
#define ENDIAN_LITTLE
//! Size of the internal EEPROM.
#define	EEPROM_SIZE			0x200
//! Size of the RAM.
#define RAM_SIZE			0x200
//! Chip ID.
#define CHIP				0x01
//! External EEPROM ID.
#define ESIZ				0x05
//! AVR architecture. Needed for assembler.
#define ARCH				avr2
#elif defined(__AVR_AT90S8535__)
//! This is a little endian architecture.
#define ENDIAN_LITTLE
//! Size of the internal EEPROM
#define	EEPROM_SIZE			0x200
//! Size of the RAM.
#define RAM_SIZE			0x200
//! Chip ID.
#define CHIP				0x01
//! External EEPROM ID.
#define ESIZ				0x03
//! AVR architecture. Needed for assembler.
#define ARCH				avr2
#elif defined(__AVR_AT90S2323__)
//! This is a little endian architecture.
#define ENDIAN_LITTLE
//! Size of the internal EEPROM
#define	EEPROM_SIZE			0x80
//! Size of the RAM.
#define RAM_SIZE			0x80
//! Chip ID.
#define CHIP				0x00
//! External EEPROM ID.
#define ESIZ				0x03
//! AVR architecture. Needed for assembler.
#define ARCH				avr2
#elif defined(__AVR_ATmega161__)
//! This is a little endian architecture.
#define ENDIAN_LITTLE
//! Size of the internal EEPROM
#define	EEPROM_SIZE			0x200
//! Size of the RAM.
#define RAM_SIZE			0x400
//! Chip ID.
#define CHIP				0x02
//! External EEPROM ID.
#define ESIZ				0x03
//! AVR architecture. Needed for assembler.
#define ARCH				avr5
#else
#error Unknown destination platform.
#endif

#endif /* PPSCS_CONFIG_H */

