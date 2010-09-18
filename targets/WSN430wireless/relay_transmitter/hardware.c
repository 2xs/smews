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


#include "target.h"

#define LFXT1CLK_HZ  32768

volatile uint32_t global_timer;

#ifndef DISABLE_RTX
interrupt (TIMERA0_VECTOR) timerA0( void ) {
	global_timer++;
}

void timer_init() {
	/* Ensure the timer is stopped. */
	TACTL = 0;
	/* Clear everything to start with. */
	TACTL |= TACLR;
	/* Run the timer of the SMCLK (1MHz). */
	TACTL = TASSEL_2;
	/* Set the compare match value according to the tick rate we want. */
	TACCR0 = 999;
	/* Enable the interrupts. */
	TACCTL0 = CCIE;

	/* ID_0                (0<<6)  Timer A input divider: 0 - /1 */
	/* ID_1                (1<<6)  Timer A input divider: 1 - /2 */
	/* ID_2                (2<<6)  Timer A input divider: 2 - /4 */
	/* ID_3                (3<<6)  Timer A input divider: 3 - /8 */
	TACTL |= ID_0;

	/* Up mode. */
	TACTL |= MC_1;
}
#endif

void xtal_init() {
	int i;
	// Init Xtal
	DCOCTL  = 0;
	BCSCTL1 = 0;
	BCSCTL2 = SELM_2 | (SELS | DIVS_3) ;

	// Wait
	do {
		IFG1 &= ~OFIFG;                  /* Clear OSCFault flag  */
		for (i = 0xff; i > 0; i--)       /* Time for flag to set */
			nop();                        /*                      */
	}  while ((IFG1 & OFIFG) != 0);    /* OSCFault flag still set? */
}

void hardware_init() {
	// WDog OFF
	WDTCTL = WDTPW + WDTHOLD;
	
	LEDS_INIT();
	xtal_init();
	dev_init();
#ifndef DISABLE_RTX
	timer_init();
#endif
	LEDS_OFF();
	eint();
}

uint16_t initADC(unsigned char channel) {
	return 1;
}

uint16_t GetADCVal(unsigned char channel) {
	ADC12CTL0 = ADC12ON | SHT0_15 | REFON; // ADC on, int. ref. on (1,5 V), multiple sample & conversion

	ADC12CTL1 = ADC12SSEL_2 | ADC12DIV_7 | CSTARTADD_0 | CONSEQ_1 | SHP;   // MCLK / 8 = 1 MHz

	ADC12MCTL0 = EOS | SREF_1 | (channel & 0x0F);           // int. ref., channel 10, last seg.

	ADC12CTL0 |= ENC;                              // enable conversion
	ADC12CTL0 |= ADC12SC;                          // sample & convert

	while (ADC12CTL0 & ADC12SC);                   // wait until conversion is complete

	ADC12CTL0 &= ~ENC;                             // disable conversion

	return ADC12MEM0;
}
