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

#ifndef __SIMPLELIB_SERIAL_H__
#define __SIMPLELIB_SERIAL_H__

#include "mbed_globals.h"
#include "interrupts.h"

/**********************************
 *    Simple Serial Managment     *
 **********************************
 * The interrupt handler is :     *
 * SERIAL_INTERRUPT_HANDLER(void) *
 * UART0 : Serial over USB        *
 * UART1 : TX p13, RX p14         *
 * UART2 : TX p28, RX p27         *
 * UART3 : TX p9,  RX p10         *
 **********************************/
 
/** Registers **/
// Serial port (Choose UARTn (0,1,2,3))
#define UART_NUMBER UART0
#define UART_BASE TOKENPASTE2(LPC_,UART_NUMBER)

// Peripheral Clock Selection registers (See 4.7.3 p56)
#define UART0_PCLK_REG (LPC_SC->PCLKSEL0)
#define UART1_PCLK_REG (LPC_SC->PCLKSEL0)
#define UART2_PCLK_REG (LPC_SC->PCLKSEL1)
#define UART3_PCLK_REG (LPC_SC->PCLKSEL1)
#define UART_PCLK_REG  TOKENPASTE2(UART_NUMBER,_PCLK_REG)

#define UART0_PCLK_OFFSET   6
#define UART1_PCLK_OFFSET   8
#define UART2_PCLK_OFFSET  16
#define UART3_PCLK_OFFSET  18
#define UART_PCLK_OFFSET   TOKENPASTE2(UART_NUMBER,_PCLK_OFFSET)

#define UART0_PCLK ((LPC_SC->PCLKSEL0 >>  6) & 0x03)
#define UART1_PCLK ((LPC_SC->PCLKSEL0 >>  8) & 0x03)
#define UART2_PCLK ((LPC_SC->PCLKSEL1 >> 16) & 0x03)
#define UART3_PCLK ((LPC_SC->PCLKSEL1 >> 18) & 0x03)
#define UART_PCLK TOKENPASTE2(UART_NUMBER,_PCLK)

// Pin Function Select register (See 8.5.1-8 p108)
#define UART0RX_PINSEL_REG (LPC_PINCON->PINSEL0)
#define UART1RX_PINSEL_REG (LPC_PINCON->PINSEL1)
#define UART2RX_PINSEL_REG (LPC_PINCON->PINSEL0)
#define UART3RX_PINSEL_REG (LPC_PINCON->PINSEL0)
#define UARTRX_PINSEL_REG  TOKENPASTE2(UART_NUMBER,RX_PINSEL_REG)

#define UART0TX_PINSEL_REG (LPC_PINCON->PINSEL0)
#define UART1TX_PINSEL_REG (LPC_PINCON->PINSEL0)
#define UART2TX_PINSEL_REG (LPC_PINCON->PINSEL0)
#define UART3TX_PINSEL_REG (LPC_PINCON->PINSEL0)
#define UARTTX_PINSEL_REG  TOKENPASTE2(UART_NUMBER,TX_PINSEL_REG)

#define UART0RX_PINSEL_OFFSET   6
#define UART1RX_PINSEL_OFFSET   0
#define UART2RX_PINSEL_OFFSET  22
#define UART3RX_PINSEL_OFFSET   2
#define UARTRX_PINSEL_OFFSET    TOKENPASTE2(UART_NUMBER,RX_PINSEL_OFFSET)

#define UART0TX_PINSEL_OFFSET   4
#define UART1TX_PINSEL_OFFSET  30
#define UART2TX_PINSEL_OFFSET  20
#define UART3TX_PINSEL_OFFSET   0
#define UARTTX_PINSEL_OFFSET    TOKENPASTE2(UART_NUMBER,TX_PINSEL_OFFSET)

#define UART0_PINSEL_VALUE    1U
#define UART1_PINSEL_VALUE    1U
#define UART2_PINSEL_VALUE    1U
#define UART3_PINSEL_VALUE    2U
#define UART_PINSEL_VALUE     TOKENPASTE2(UART_NUMBER,_PINSEL_VALUE)

/** Interrupt handlers **/
#define SERIAL_INTERRUPT_HANDLER EXTERN_C void __IRQ TOKENPASTE2(UART_NUMBER,_IRQHandler)

/** Bits **/
// RBR Interrupt Enable (UnIER, 14.4.4 p302)
#define RBR_INT_BIT 0
// Receiver Data Ready (UnLSR, 14.4.8 p306)
#define RDR_BIT 0
// Transmitter Holding Register Empty (UnLSR, 14.4.8 p306)
#define THRE_BIT 5
// RBR Interrupt Enable (UnIER, 14.4.4 p302)
#define SERIAL_INT_RX 1
// THRE Interrupt Enable (UnIER, 14.4.4 p302)
#define SERIAL_INT_TX 2
// Divisor Latch Access Bit (UnLCR, 14.4.7 p306)
#define DLA_BIT 7
// Power Control for Peripherals (PCONP, 4.8.7.1 p63)
#define UART0_PCONP_BIT  3
#define UART1_PCONP_BIT  4
#define UART2_PCONP_BIT 24
#define UART3_PCONP_BIT 25

/** Macros **/
#define SERIAL_PUTCHAR(c)               do {                                                        \
                                            while (GET_BIT_VALUE(UART_BASE->LSR, THRE_BIT) == 0);   \
                                            UART_BASE->THR = c;                                     \
                                        } while(0)

#define SERIAL_DATA_TO_READ()           (GET_BIT_VALUE(UART_BASE->LSR, RDR_BIT) == 1)

#define SERIAL_GETCHAR()                (UART_BASE->RBR)

// Enable interrupt for RX or TX (SERIAL_INT_RX and SERIAL_INT_TX)
#define SERIAL_ENABLE_INTERRUPT(value)  do {                                                    \
                                            UART_BASE->IER = value;                             \
                                            ENABLE_INTERRUPT(TOKENPASTE2(UART_NUMBER,_IRQn));   \
                                        } while(0)

extern __INLINE void SERIAL_INIT()
{
    // Enable UARTn
    SET_BIT_VALUE(LPC_SC->PCONP, TOKENPASTE2(UART_NUMBER,_PCONP_BIT) , 1);
    // Enable FIFO and reset RX/TX FIFO (See 14.4.6 p305)
    UART_BASE->FCR = 0x07;
    // 8-bits, No Parity, 1 stop bit (See 14.4.7 p306)
    UART_BASE->LCR = 0x03;
    // Set CCLK as Peripheral Clock for UART (96MHz with mbed library)
    UART_PCLK_REG = (UART_PCLK_REG & (~(3UL << UART_PCLK_OFFSET))) | (1U << UART_PCLK_OFFSET);
    // Define Pin's functions as UART
    UARTRX_PINSEL_REG = (UARTRX_PINSEL_REG & (~(3U << UARTRX_PINSEL_OFFSET))) | (UART_PINSEL_VALUE << UARTRX_PINSEL_OFFSET);
    UARTTX_PINSEL_REG = (UARTTX_PINSEL_REG & (~(3U << UARTTX_PINSEL_OFFSET))) | (UART_PINSEL_VALUE << UARTTX_PINSEL_OFFSET);
}

// See 14.4.5 p303
extern __INLINE int SERIAL_CHECK_INTERRUPT(void) {
    uint32_t serialStatus = UART_BASE->IIR;

    if (serialStatus & 1) // IntStatus, 1 = No Interrupt is pending.
        return 0;

    serialStatus = (serialStatus >> 1) & 0x3; // IntId, 2 = More than threshold data to read, 6 = Some caracters to read
    if (serialStatus != 2 && serialStatus != 6)
        return 0;

    return 1;
}

extern __INLINE void SERIAL_SETBAUD(unsigned int baud) {
    // Peripheral Clock Selection register bit values (See Table 42, p57)
    uint16_t divisorValue = (SystemCoreClock / 16 / baud);
    #if 0
        // Peripheral Clock for UART is set to CCLK in SERIAL_INIT. Divisor is then 1.
        // Else, use code below
        static int divisors[4] = { 4, 1, 2, 8 };
        uint16_t divisorValue = ((SystemCoreClock / 16 / baud) / divisors[UART_PCLK]);
    #endif
    
    UART_BASE->LCR |= (1 << DLA_BIT);
    UART_BASE->DLM = (uint8_t) (divisorValue >> 8);
    UART_BASE->DLL = (uint8_t)  divisorValue;
    UART_BASE->LCR &= ~(1 << DLA_BIT);
}

#endif