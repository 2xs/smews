/*
 * Copyright  2008-2009 INRIA/SensTools
 * 
 * <dev-team@sentools.info>
 * 
 * This software is a set of libraries designed to develop applications
 * for the WSN430 embedded hardware platform.
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
/**
 * \defgroup spi1 SPI1 driver
 * \ingroup wsn430
 * @{
 * 
 * The SPI1 driver allows SPI communication
 * using the USART1 block of the MSP430. This driver is mainly
 * used by other peripheral devices' drivers.
 * 
 * It defines macros to initialize the SPI module,
 * to chip select/deselect the three default hardware
 * peripherals connected by SPI to the MSP430,
 * to write/read bytes to/from these peripherals.
 * 
 * Note that when chip selecting a device,
 * the driver automatically deselects
 * the two other ones to avoid bus collision.
 *
 */

/**
 * \file
 * \brief SPI1 driver.
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date November 08
 */

#ifndef SPI1_H
#define SPI1_H

extern uint8_t spi1_tx_return_value;

enum {
    SPI1_CC1100 = 1,
    SPI1_CC2420 = 1,
    SPI1_DS1722 = 2,
    SPI1_M25P80 = 3
};

/**
 * Initialize the UART1 for SPI use.
 */
void spi1_init(void);

uint8_t spi1_write_single(uint8_t byte);
uint8_t spi1_read_single(void);

uint8_t spi1_write(uint8_t* data, int16_t len);
void spi1_read(uint8_t* data, int16_t len);

void spi1_select(int16_t chip);
void spi1_deselect(int16_t chip);

int16_t spi1_read_somi(void);

/**
 * @}
 */

#endif
