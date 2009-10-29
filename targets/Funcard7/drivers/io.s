; Copyright or © or Copr. 2008, Geoffroy Cogniaux, Gilles Grimaud,
; Simon Duquennoy
; 
; This software is a computer program whose purpose is to design an
; efficient Web server for very-constrained embedded system.
; 
; This software is governed by the CeCILL license under French law and
; abiding by the rules of distribution of free software.  You can  use, 
; modify and/ or redistribute the software under the terms of the CeCILL
; license as circulated by CEA, CNRS and INRIA at the following URL
; "http://www.cecill.info". 
; 
; As a counterpart to the access to the source code and  rights to copy,
; modify and redistribute granted by the license, users are provided only
; with a limited warranty  and the software's author,  the holder of the
; economic rights,  and the successive licensors  have only  limited
; liability. 
; 
; In this respect, the user's attention is drawn to the risks associated
; with loading,  using,  modifying and/or developing or reproducing the
; software by the user in light of its specific status of free software,
; that may mean  that it is complicated to manipulate,  and  that  also
; therefore means  that it is reserved for developers  and  experienced
; professionals having in-depth computer knowledge. Users are therefore
; encouraged to load and test the software's suitability as regards their
; requirements in conditions enabling the security of their systems and/or 
; data to be ensured and,  more generally, to use and operate it in the 
; same conditions as regards security. 
; 
; The fact that you are presently reading this means that you have had
; knowledge of the CeCILL license and that you accept its terms.
;

; Parts of code from SOSSE:

; Simple Operating system for Smart cards
; Copyright (C) 2002  Matthias Bruestle <m@mbsks.franken.de>
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

;========================================================================
; T=0 character I/O routines for 9600bps at 3.58 MHz
;========================================================================

#include "at_config.h"

.arch	avr2; should be replace by ARCH
IO_PIN=6
PINB=0x16
DDRB=0x17
PORTB=0x18
	.text
	.global dropByteT0, sendbyteT0, sendFbyteT0, recFbyteT0
	.comm	direction,1,1
;========================================================================
; Wait loops.
; Wait t17*3+7 cycles
delay:
	dec		r22				; 1
	brne	delay			; 1/2
	ret						; 4

delay1etu:
	ldi		r22, 121		; 1
	rjmp	delay			; 2

;========================================================================
; Receive a byte with T=0 error correction.
; result r25(=0):r24
dropByteT0:
;	push	r23				; 2 - getbit
;	push	r22				; 2 - delay
;	push	r21				; 2 - loop counter
;	push	r20				; 2 - parity counter

	; Set direction bit, to indicate, that we received a byte
	ldi		r22, 1
	sts		direction,r22

	; Setup IN direction
	cbi		DDRB, 6			; 2
	cbi		PORTB, 6		; 2

; Wait for start bit.
waitforstart:
	; Bit begins here.
	sbic	PINB, IO_PIN	; 1/2!
	rjmp	waitforstart	; 2/0
	sbic	PINB, IO_PIN	; 1/2! - Recheck for spike
	rjmp	waitforstart	; 2/0

	ldi		r23, 5			; 1
delay_loop:
	ldi	r22, 245			; 1
	rcall	delay			; 742
	dec	r23				; 1
	brne	delay_loop			; 1/2

;	pop		r20				; 2 - parity counter
;	pop		r21				; 2 - loop counter
;	pop		r22				; 2 - delay
;	pop		r23				; 2 - getbit
	ret

;========================================================================
; Receive a fast byte with T=0 error correction.
; result r25(=0):r24
recFbyteT0:
;	push	r23				; 2 - getbit
;	push	r22				; 2 - delay
;	push	r21				; 2 - loop counter
;	push	r20				; 2 - parity counter

	; Set direction bit, to indicate, that we received a byte
	ldi		r22, 1
	sts		direction,r22

restartrecFbyte:
	; Setup IN direction
	cbi		DDRB, 6			; 2
	cbi		PORTB, 6		; 2

; Wait for start bit.
waitforstartF:
	; Bit begins here.
	sbic	PINB, IO_PIN	; 1/2!
	rjmp	waitforstartF	; 2/0
	sbic	PINB, IO_PIN	; 1/2! - Recheck for spike
	rjmp	waitforstartF	; 2/0
	; Sample start bit
	clr		r24				; 1
	clr		r25				; 1 - Clear zero byte for ADC
	nop
	nop
	nop						; 3
	rcall	getFbit			; 3 (16bit PC)
	;brcs	waitforstart	; 1/2 - Go on, even if not valid a start bit?
	nop						; 1 - For brcs
; Receive now 9 bits
	ldi		r21, 0x09		; 1
	clr		r20				; 1
	ldi		r22, 66			; 1
	nop						; 1
	nop						; 1
rnextbitF:
    rcall	wait10clock		; 3
	rcall	getFbit			; 3
	add		r20, r23		; 1
	clc						; 1
	sbrc	r23, 0			; 1/2
	sec						; 1/0
	ror		r24				; 1
	ldi		r22, 65			; 1
	dec		r21				; 1
	brne	rnextbitF		; 1/2
; Check parity
	rol		r24				; 1 - We've rotated one to much
	sbrc	r20, 0			; 1/2
	rjmp	regetbyteF		; 2/0

	; Wait half etu
	ldi		r22, 3		; 1
	rcall	delay			; 16
	nop 				; 1

	clr		r25
;	pop		r20				; 2 - parity counter
;	pop		r21				; 2 - loop counter
;	pop		r22				; 2 - delay
;	pop		r23				; 2 - getbit
	ret

wait11clock:
	nop						; 1
wait10clock:
	nop						; 1
	nop						; 1
	nop						; 1
	ret						; 4

regetbyteF:
	; Wait half etu
	ldi		r22, 3		; 1
	rcall	delay			; 16
	nop 				; 1
	; Set OUT direction
	sbi		DDRB, 6			; 2
	; Signal low
	cbi		PORTB, 6		; 2
	ldi		r22, 15		; 1
	rcall	delay			; 52
	nop				; 1 -about 1.5 etu
	rjmp	restartrecFbyte	; 2
;========================================================================
; Read a bit.
; Uses r23, r25
; Returns bit in r23.0.
; 3 cycles before first bit
; 6 cycles after last bit.
getFbit:
	clr		r23				; 1
	clc						; 1
	; At start + 112 cycles
	sbic	PINB, IO_PIN	; 1/2
	sec						; 1/0
	adc		r23, r25		; 1
	ret						; 4	(with 16bit PC)	
;========================================================================
; Send a byte with T=0 error correction.
; byte r25(=0):r24
sendbyteT0:
;	push	r22				; 2 - delay
;	push	r23				; 2 - parity counter

	lds		r22,direction
	tst		r22
	breq	resendbyteT0
	rcall	delay1etu		;
	rcall	delay1etu		;
	; Clear direction bit, to indicate, that we sent a byte
	ldi		r22, 0
	sts		direction,r22

resendbyteT0:
	; Set OUT direction
	sbi		PORTB, 6		; 2
	sbi		DDRB, 6			; 2
	; Send start bit
	cbi		PORTB, IO_PIN	; 2
	ldi		r22, 119		; 1
	rcall	delay			; 364
	; Send now 8 bits
	ldi		r25, 0x08		; 1
	clr		r23				; 1
snextbit:
	ror		r24				; 1
	brcs	sendbit1		; 1/2
	cbi		PORTB, IO_PIN	; 2
	rjmp	bitset			; 2
sendbit1:
	sbi		PORTB, IO_PIN	; 2
	inc		r23				; 1
bitset:
	ldi		r22, 118		; 1
	rcall	delay			; 361
	nop						; 1
	dec		r25				; 1
	brne	snextbit		; 1/2
	; Send parity
	sbrc	r23, 0			; 1/2
	rjmp	sendparity1		; 2
	nop						; 1
	nop						; 1
	cbi		PORTB, IO_PIN	; 2
	rjmp	delayparity		; 2
sendparity1:
	nop						; 1
	sbi		PORTB, IO_PIN	; 2
	nop						; 1
	nop						; 1
delayparity:
	ldi		r22, 112		; 1
	rcall	delay			; 343
	; Stop bit
	sbi		PORTB, IO_PIN	; 2
	ldi		r22, 119		; 1
	rcall	delay			; 364
	; Set IN direction
	cbi		DDRB, 6			; 2
	cbi		PORTB, 6		; 2
	; Look for error signal
	clc						; 1
	sbic	PINB, IO_PIN	; 1/2
	sec						; 1/0
	brcs	retsendbyteT0	; 1/2
	; Resend byte
	; Bring byte to starting position
	ror		r24				; 1
	; Wait for end of error signal
waitforendoferror:
	sbic	PINB, IO_PIN	; 1/2!
	rjmp	waitforendoferror	; 2/0
	; Wait then a half etu
	ldi		r22, 58			; 1
	rcall	delay			; 181
	rjmp	resendbyteT0	; 2
	; return
retsendbyteT0:
	ldi		r22, 116		; 1
	rcall	delay			; 355
;	pop		r23				; 2 - parity counter
;	pop		r22				; 2 - delay
	ret						; 4
;========================================================================

;========================================================================
; Send a byte with T=0 error correction.
; byte r25(=0):r24
sendFbyteT0:
;	push	r22				; 2 - delay
;	push	r23				; 2 - parity counter

	lds		r22,direction
	tst		r22
	breq	resendFbyteT0
	ldi		r22, 18		; 1
	rcall	delay			; 61
	; Clear direction bit, to indicate, that we sent a byte
	ldi		r22, 0
	sts		direction,r22

resendFbyteT0:
	; Set OUT direction
	sbi		PORTB, 6		; 2
	sbi		DDRB, 6			; 2
	; Send start bit
	cbi		PORTB, IO_PIN	; 2
	ldi		r22, 5		; 1
	rcall	delay			; 22
	nop				; 1
	; Send now 8 bits
	ldi		r25, 0x08		; 1
	clr		r23				; 1
sFnextbit:
	ror		r24				; 1
	brcs	sendFbit1		; 1/2
	cbi		PORTB, IO_PIN	; 2
	rjmp	bitFset			; 2
sendFbit1:
	sbi		PORTB, IO_PIN	; 2
	inc		r23				; 1
bitFset:
	ldi		r22, 5		; 1
	rcall	delay			; 22
	dec		r25				; 1
	brne	sFnextbit		; 1/2
	; Send parity
	sbrc	r23, 0			; 1/2
	rjmp	sendFparity1	; 2
	nop						; 1
	nop						; 1
	cbi		PORTB, IO_PIN	; 2
	rjmp	delayFparity	; 2
sendFparity1:
	nop						; 1
	sbi		PORTB, IO_PIN	; 2
	nop						; 1
	nop						; 1
delayFparity:
	ldi		r22, 5		; 1
	rcall	delay			; 22
	; Stop bit
	sbi		PORTB, IO_PIN	; 2
	ldi		r22, 5		; 1
	rcall	delay			; 22
	nop				; 1
	; Set IN direction
	cbi		DDRB, 6			; 2
	cbi		PORTB, 6		; 2
	; Look for error signal
	clc						; 1
	sbic	PINB, IO_PIN	; 1/2
	sec						; 1/0
	brcs	retsendFbyteT0	; 1/2
	; Resend byte
	; Bring byte to starting position
	ror		r24				; 1
	; Wait for end of error signal
waitforendofFerror:
	sbic	PINB, IO_PIN	; 1/2!
	rjmp	waitforendofFerror	; 2/0
	; Wait then a half etu
	ldi		r22, 3		; 1
	rcall	delay			; 16
	nop 				; 1
	rjmp	resendFbyteT0	; 2
	; return
retsendFbyteT0:
	rcall   wait11clock     ; 3
;	pop		r23				; 2 - parity counter
;	pop		r22				; 2 - delay
	ret						; 4
;========================================================================
