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

#ifndef __SIMPLELIB_INTERRUPTS_H__
#define __SIMPLELIB_INTERRUPTS_H__

#include "mbed_globals.h"

/** Interrupt Managment **/
#define ENABLE_INTERRUPT(intr)  NVIC_EnableIRQ(intr)
#define DISABLE_INTERRUPT(intr) NVIC_DisableIRQ(intr)

#if defined ( __CC_ARM )
    #define __IRQ     __irq
#elif defined   ( __GNUC__ )
    #define __IRQ     __attribute__((interrupt("IRQ")))
#endif

/* Interrupts names
 * WDT_IRQn         Watchdog Timer Interrupt
 * TIMER0_IRQn      Timer0 Interrupt
 * TIMER1_IRQn      Timer1 Interrupt
 * TIMER2_IRQn      Timer2 Interrupt
 * TIMER3_IRQn      Timer3 Interrupt
 * UART0_IRQn       UART0 Interrupt
 * UART1_IRQn        UART1 Interrupt
 * UART2_IRQn        UART2 Interrupt
 * UART3_IRQn        UART3 Interrupt
 * PWM1_IRQn        PWM1 Interrupt
 * I2C0_IRQn        I2C0 Interrupt
 * I2C1_IRQn        I2C1 Interrupt
 * I2C2_IRQn        I2C2 Interrupt
 * SPI_IRQn         SPI Interrupt
 * SSP0_IRQn        SSP0 Interrupt
 * SSP1_IRQn        SSP1 Interrupt
 * PLL0_IRQn        PLL0 Lock (Main PLL) Interrupt
 * RTC_IRQn         Real Time Clock Interrupt
 * EINT0_IRQn        External Interrupt 0 Interrupt
 * EINT1_IRQn        External Interrupt 1 Interrupt
 * EINT2_IRQn        External Interrupt 2 Interrupt
 * EINT3_IRQn        External Interrupt 3 Interrupt
 * ADC_IRQn         A/D Converter Interrupt
 * BOD_IRQn            Brown-Out Detect Interrupt
 * USB_IRQn            USB Interrupt
 * CAN_IRQn            CAN Interrupt
 * DMA_IRQn            General Purpose DMA Interrupt
 * I2S_IRQn            I2S Interrupt
 * ENET_IRQn        Ethernet Interrupt
 * RIT_IRQn            Repetitive Interrupt Timer Interrupt
 * MCPWM_IRQn        Motor Control PWM Interrupt
 * QEI_IRQn            Quadrature Encoder Interface Interrupt
 * PLL1_IRQn        PLL1 Lock (USB PLL) Interrupt
 */

/* Default interrupt handlers
 * WDT_IRQHandler
 * TIMER0_IRQHandler
 * TIMER1_IRQHandler
 * TIMER2_IRQHandler
 * TIMER3_IRQHandler
 * UART0_IRQHandler
 * UART1_IRQHandler
 * UART2_IRQHandler
 * UART3_IRQHandler
 * PWM1_IRQHandler
 * I2C0_IRQHandler
 * I2C1_IRQHandler
 * I2C2_IRQHandler
 * SPI_IRQHandler
 * SSP0_IRQHandler
 * SSP1_IRQHandler
 * PLL0_IRQHandler
 * RTC_IRQHandler
 * EINT0_IRQHandler
 * EINT1_IRQHandler
 * EINT2_IRQHandler
 * EINT3_IRQHandler
 * ADC_IRQHandler
 * BOD_IRQHandler
 * USB_IRQHandler
 * CAN_IRQHandler
 * DMA_IRQHandler
 * I2S_IRQHandler
 * ENET_IRQHandler
 * RIT_IRQHandler
 * MCPWM_IRQHandler
 * QEI_IRQHandler
 * PLL1_IRQHandler
*/ 
 
#endif