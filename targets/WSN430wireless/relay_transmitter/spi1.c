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
 *addtogroup wsn430
 * @{
 */

/**
 *addtogroup spi1
 * @{
 */

/**
 *file
 *brief SPI1 driver
 *author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 *date October 09
 */

/**
 * @}
 */

/**
 * @}
 */

#include <io.h>
#include "spi1.h"

/* Local Macros */
/*
 * wait until a byte has been received on spi port
 */
#define WAIT_EORX() while ( (IFG2 & URXIFG1) == 0){}

/*
 * wait until a byte has been sent on spi port
 */
#define WAIT_EOTX() while ( (IFG2 & UTXIFG1) == 0){}

#define CC1100_CS_PIN (1<<2)
#define CC2420_CS_PIN (1<<2)
#define DS1722_CS_PIN (1<<3)
#define M25P80_CS_PIN (1<<4)

#define CC1100_ENABLE()  P4OUT &= ~CC1100_CS_PIN
#define CC1100_DISABLE() P4OUT |=  CC1100_CS_PIN

#define CC2420_ENABLE()  P4OUT &= ~CC2420_CS_PIN
#define CC2420_DISABLE() P4OUT |=  CC2420_CS_PIN

#define DS1722_ENABLE()  P4OUT |=  DS1722_CS_PIN
#define DS1722_DISABLE() P4OUT &= ~DS1722_CS_PIN

#define M25P80_ENABLE()  P4OUT &= ~M25P80_CS_PIN
#define M25P80_DISABLE() P4OUT |=  M25P80_CS_PIN

void spi1_init(void) {
    /* Configure IO pins */
    P5DIR  |=   (1<<1) | (1<<3); /* output for CLK and SIMO */
    P5DIR  &=  ~(1<<2);   /* input for SOMI */
    P5SEL  |=   (1<<1) | (1<<2) | (1<<3); /* SPI for all three */
    
    /* Configure USART1 */
    U1CTL = SWRST; /* SPI 1 software reset */
    U1CTL = CHAR | SYNC | MM | SWRST;  /* 8bit SPI master */
    U1TCTL = CKPH | SSEL_2 | STC;    /* clock delay, SMCLK */

    U1RCTL = 0; /* clear errors */
    U1BR0 = 0x2; /* baudrate = SMCLK/2 */
    U1BR1 = 0x0;

    ME2 |= USPIE1; /* enable SPI module */
    IE2 &= ~(UTXIE1 | URXIE1); /* disable SPI interrupt */
    U1CTL &= ~(SWRST); /* clear reset */

    /* CS IO pins configuration */
    P4SEL &= ~(CC1100_CS_PIN | DS1722_CS_PIN | M25P80_CS_PIN);
    P4DIR |=  (CC1100_CS_PIN | DS1722_CS_PIN | M25P80_CS_PIN);

    /* disable peripherals */
    M25P80_DISABLE();
    CC1100_DISABLE();
    DS1722_DISABLE();
}

uint8_t spi1_write_single(uint8_t byte) {
    uint8_t dummy;
    U1TXBUF = byte;
    WAIT_EORX();
    dummy = U1RXBUF;
    
    return dummy;
}

uint8_t spi1_read_single(void) {
    return spi1_write_single(0x0);
}

uint8_t spi1_write(uint8_t* data, int16_t len) {
    uint8_t dummy=0;
    int16_t i;
    
    for (i=0; i<len; i++) {
        U1TXBUF = data[i];
        WAIT_EORX();
        dummy = U1RXBUF;
    }
    return dummy;
}
void spi1_read(uint8_t* data, int16_t len) {
    int16_t i;
    
    for (i=0; i<len; i++) {
        U1TXBUF = 0x0;
        WAIT_EORX();
        data[i] = U1RXBUF;
    }
}

void spi1_select(int16_t chip) {
    switch (chip) {
    case SPI1_CC1100:
        M25P80_DISABLE();
        DS1722_DISABLE();
        CC1100_ENABLE();
        break;
    case SPI1_DS1722:
        M25P80_DISABLE();
        CC1100_DISABLE();
        DS1722_DISABLE();
        U1CTL |= SWRST;
        U1TCTL &= ~(CKPH);
        U1CTL &= ~(SWRST);
        DS1722_ENABLE();
        break;
    case SPI1_M25P80:    
        CC1100_DISABLE();
        DS1722_DISABLE();
        M25P80_ENABLE();
        break;
    default:
        break;
    }
}

void spi1_deselect(int16_t chip) {
    switch (chip) {
    case SPI1_CC1100:
        CC1100_DISABLE();
        break;
    case SPI1_DS1722:
        DS1722_DISABLE();
        U1CTL |= SWRST;
        U1TCTL |= CKPH;
        U1CTL &= ~(SWRST);
        break;
    case SPI1_M25P80:
        M25P80_DISABLE();
        break;
    default:
        break;
    }
}

int16_t spi1_read_somi(void) {
    return P5IN & (1<<2);
}
