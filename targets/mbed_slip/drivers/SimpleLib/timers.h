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

#ifndef __SIMPLELIB_TIMERS_H__
#define __SIMPLELIB_TIMERS_H__

#include "mbed_globals.h"
#include "interrupts.h"

/**********************************
 *    Simple Timers Managment     *
 **********************************
 * The interrupt handler is :     *
 * TIMERn_INTERRUPT_HANDLER(void) *
 **********************************/

/** Registers **/
#define TIMER0_BASE (LPC_TIM0)
#define TIMER1_BASE (LPC_TIM1)
#define TIMER2_BASE (LPC_TIM2)
#define TIMER3_BASE (LPC_TIM3)
#define TIMER_BASE(timer) TOKENPASTE2(timer,_BASE)

// Peripheral Clock Selection registers (See 4.7.3 p56)
#define TIMER0_PCLK_REG (LPC_SC->PCLKSEL0)
#define TIMER1_PCLK_REG (LPC_SC->PCLKSEL0)
#define TIMER2_PCLK_REG (LPC_SC->PCLKSEL1)
#define TIMER3_PCLK_REG (LPC_SC->PCLKSEL1)
#define TIMER_PCLK_REG(timer)  TOKENPASTE2(timer,_PCLK_REG)

#define TIMER0_PCLK_OFFSET   2
#define TIMER1_PCLK_OFFSET   4
#define TIMER2_PCLK_OFFSET  12
#define TIMER3_PCLK_OFFSET  14
#define TIMER_PCLK_OFFSET(timer)   TOKENPASTE2(timer,_PCLK_OFFSET)

/** Interrupt handlers **/
#define TIMER0_INTERRUPT_HANDLER TIMER_INTERRUPT_HANDLER(TIMER0)
#define TIMER1_INTERRUPT_HANDLER TIMER_INTERRUPT_HANDLER(TIMER1)
#define TIMER2_INTERRUPT_HANDLER TIMER_INTERRUPT_HANDLER(TIMER2)
#define TIMER3_INTERRUPT_HANDLER TIMER_INTERRUPT_HANDLER(TIMER3)
#define TIMER_INTERRUPT_HANDLER(timer) EXTERN_C void __IRQ TOKENPASTE2(timer,_IRQHandler)

/** Bits **/
// Power Control for Peripherals (PCONP, 4.8.7.1 p63)
#define TIMER0_PCONP_BIT  1
#define TIMER1_PCONP_BIT  2
#define TIMER2_PCONP_BIT 22
#define TIMER3_PCONP_BIT 23

// Match Control Register (TnMCR, 21.6.8 p496)
#define MATCH_INTERRUPT   1
#define MATCH_RESET       2
#define MATCH_STOP        4
#define MR0_OFFSET        0
#define MR1_OFFSET        3
#define MR2_OFFSET        6
#define MR3_OFFSET        9

// Interrupt Register (TnIR, 21.6.1, p493)
#define MR0_INT    (1U << 0)
#define MR1_INT    (1U << 1)
#define MR2_INT    (1U << 2)
#define MR3_INT    (1U << 3)
#define CR0_INT    (1U << 4)
#define CR1_INT    (1U << 5)

/** Macros **/
// Enable TIMERn
#define TIMER0_INIT() TIMER_INIT(TIMER0)
#define TIMER1_INIT() TIMER_INIT(TIMER1)
#define TIMER2_INIT() TIMER_INIT(TIMER2)
#define TIMER3_INIT() TIMER_INIT(TIMER3)
#define TIMER_INIT(timer)   do {                                                                                                   \
                                SET_BIT_VALUE(LPC_SC->PCONP, TOKENPASTE2(timer,_PCONP_BIT) , 1); /* Enable Timer                */ \
                                TIMER_BASE(timer)->TCR = 0x2;                                    /* Reset Timer, Table 427 p493 */ \
                            } while(0)

// Set Peripheral Clock
#define TIMER0_SETPCLK(clk) TIMER_SETPCLK(TIMER0, clk)
#define TIMER1_SETPCLK(clk) TIMER_SETPCLK(TIMER1, clk)
#define TIMER2_SETPCLK(clk) TIMER_SETPCLK(TIMER2, clk)
#define TIMER3_SETPCLK(clk) TIMER_SETPCLK(TIMER3, clk)
#define TIMER_SETPCLK(timer, clk)  TIMER_PCLK_REG(timer) = ((TIMER_PCLK_REG(timer) & (~(3U << TIMER_PCLK_OFFSET(timer)))) | (clk << TIMER_PCLK_OFFSET(timer)))

// Set Prescale Register
#define TIMER0_SETPRESCALE(value) TIMER_SETPRESCALE(TIMER0, value)
#define TIMER1_SETPRESCALE(value) TIMER_SETPRESCALE(TIMER1, value)
#define TIMER2_SETPRESCALE(value) TIMER_SETPRESCALE(TIMER2, value)
#define TIMER3_SETPRESCALE(value) TIMER_SETPRESCALE(TIMER3, value)
#define TIMER_SETPRESCALE(timer, value)  TIMER_BASE(timer)->PR = (value)

// Set Match Register (MR0-3, 21.6.7 p496)
#define TIMER0_SETMATCH(id, value) TIMER_SETMATCH(TIMER0, id, value)
#define TIMER1_SETMATCH(id, value) TIMER_SETMATCH(TIMER1, id, value)
#define TIMER2_SETMATCH(id, value) TIMER_SETMATCH(TIMER2, id, value)
#define TIMER3_SETMATCH(id, value) TIMER_SETMATCH(TIMER3, id, value)
#define TIMER_SETMATCH(timer, id, value)  TIMER_BASE(timer)->TOKENPASTE2(MR,id) = (value)

// Set Match Control Register (TnMCR, 21.6.8 p496)
#define TIMER0_SETMATCHCONTROL(id, value) TIMER_SETMATCHCONTROL(TIMER0, id, value)
#define TIMER1_SETMATCHCONTROL(id, value) TIMER_SETMATCHCONTROL(TIMER1, id, value)
#define TIMER2_SETMATCHCONTROL(id, value) TIMER_SETMATCHCONTROL(TIMER2, id, value)
#define TIMER3_SETMATCHCONTROL(id, value) TIMER_SETMATCHCONTROL(TIMER3, id, value)
#define TIMER_SETMATCHCONTROL(timer, id, value)  TIMER_BASE(timer)->MCR = (value) << (MR ## id ## _OFFSET)

// Enable interrupt for TIMERn
#define TIMER0_ENABLE_INTERRUPT() TIMER_ENABLE_INTERRUPT(TIMER0)
#define TIMER1_ENABLE_INTERRUPT() TIMER_ENABLE_INTERRUPT(TIMER1)
#define TIMER2_ENABLE_INTERRUPT() TIMER_ENABLE_INTERRUPT(TIMER2)
#define TIMER3_ENABLE_INTERRUPT() TIMER_ENABLE_INTERRUPT(TIMER3)
#define TIMER_ENABLE_INTERRUPT(timer)  ENABLE_INTERRUPT(TOKENPASTE2(timer,_IRQn))

// Interrut Register (TnIR, 21.6.1, p493)
#define TIMER0_CLEAR_INTERRUPT(value) TIMER_CLEAR_INTERRUPT(TIMER0, value)
#define TIMER1_CLEAR_INTERRUPT(value) TIMER_CLEAR_INTERRUPT(TIMER1, value)
#define TIMER2_CLEAR_INTERRUPT(value) TIMER_CLEAR_INTERRUPT(TIMER2, value)
#define TIMER3_CLEAR_INTERRUPT(value) TIMER_CLEAR_INTERRUPT(TIMER3, value)
#define TIMER_CLEAR_INTERRUPT(timer, value)  TIMER_BASE(timer)->IR = (value)

// Start Timer
#define TIMER0_START() TIMER_START(TIMER0)
#define TIMER1_START() TIMER_START(TIMER1)
#define TIMER2_START() TIMER_START(TIMER2)
#define TIMER3_START() TIMER_START(TIMER3)
#define TIMER_START(timer)  TIMER_BASE(timer)->TCR = 0x1 /* Counter Enable, Table 427 p493*/

// Get Timer Value
#define TIMER0_VALUE() TIMER_VALUE(TIMER0)
#define TIMER1_VALUE() TIMER_VALUE(TIMER1)
#define TIMER2_VALUE() TIMER_VALUE(TIMER2)
#define TIMER3_VALUE() TIMER_VALUE(TIMER3)
#define TIMER_VALUE(timer)  (TIMER_BASE(timer)->TC)

#endif