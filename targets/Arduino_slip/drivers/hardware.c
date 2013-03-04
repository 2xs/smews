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
#include <avr/interrupt.h>
#include <avr/io.h>

#define AVR_TIMER_CTC (_BV(WGM01))
#define AVR_TIMER_CLK_0 0x00
#define AVR_TIMER_CLK_1 (_BV(CS00))
#define AVR_TIMER_CLK_8 (_BV(CS01))
#define AVR_TIMER_CLK_32 (_BV(CS01) | _BV(CS00))
#define AVR_TIMER_CLK_64 (_BV(CS02))
#define AVR_TIMER_CLK_128 (_BV(CS02) | _BV(CS00))
#define AVR_TIMER_CLK_256 (_BV(CS02) | _BV(CS01))
#define AVR_TIMER_CLK_1024 (_BV(CS02) | _BV(CS01) | _BV(CS00))

#define ADC_PROC_REF_AREF     (0x00)
#define ADC_PROC_REF_AVCC     _BV(REFS0)
#define ADC_PROC_REF_RSRVD    _BV(REFS1)
#define ADC_PROC_REF_INTERNAL (_BV(REFS1)|_BV(REFS0))

#define ADC_PROC_CH1 0x01
#define ADC_PROC_GND 0x1F

#define ADC_PROC_CLK_2   (_BV(ADPS0))
#define ADC_PROC_CLK_4   (_BV(ADPS1))
#define ADC_PROC_CLK_8   (_BV(ADPS1)|_BV(ADPS0))
#define ADC_PROC_CLK_16   (_BV(ADPS2))
#define ADC_PROC_CLK_32   (_BV(ADPS2)|_BV(ADPS0))
#define ADC_PROC_CLK_64   (_BV(ADPS2)|_BV(ADPS1))
#define ADC_PROC_CLK_128   (_BV(ADPS2)|_BV(ADPS1)|_BV(ADPS0))

#define PRESCALER	1024
#define TIME_SLOT	1 // Time slot in milliseconds
#define NB_TICK		(((F_CPU/PRESCALER)*TIME_SLOT)/1000)

volatile uint32_t global_timer;


ISR(TIMER1_COMPA_vect)
{
	cli();
	global_timer++;
	sei();
}


void hardware_timer_init(void) {

	global_timer = 0;
	
	TCCR1B |= _BV(WGM12); // CTC mode with value in OCR1A 
	TCCR1B |= _BV(CS12);  // CS12 = 1; CS11 = 0; CS10 =1 => CLK/1024 prescaler
	TCCR1B |= _BV(CS10);
	OCR1A   = NB_TICK;
	TIMSK1 |= _BV(OCIE1A);
}

void hardware_init() {
	hardware_timer_init();
	dev_init();
	sei();
}

