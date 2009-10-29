; Copyright or © or Copr. 2008, Geoffroy Cogniaux, Simon Duquennoy,
; Thomas Soete
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

#include "at_config.h"

.arch	avr2; should be replace by ARCH

DDRB=0x17
EE_SCL=1
EE_SDA=0
PINB=0x16
PORTB=0x18
	.text
	.global xewrt, fxeread, fxeread_next
XEAddr:
	rcall  XEStrt
	clc
	ldi	   r19,0xA0
	rcall  XEEOut
	rcall  XE0Bit
	mov	   r19,r31
	rcall  XEEOut
	rcall  XE0Bit
	mov	   r19,r30
	rcall  XEEOut
	rcall  XE0Bit
	ret

; address r25:r24 
; byte r23(=0):r22
xewrt:
	push	r29
	push	r28
	push	r16
	push	r1
	push	r0
	mov	r31,r25
	mov	r30,r24
; Start
; address r31:r30 
; result XE(Z+) = r22
;	rcall	xereadlocal
;	cp	r0,r22
;	breq	dontwrite
	rcall	XE1Bit
	rcall	XEStop
	ldi r28,lo8(-1)
	ldi r29,hi8(-1)
	sts last_addr,r28
	sts (last_addr)+1,r29
	rcall	XEAddr
	mov	r19,r22
	rcall	XEEOut
	rcall	XE0Bit
	rcall	XEStop
	rcall	XEDly
dontwrite: 
; Done
	pop	r0
	pop	r1
	pop	r16
	pop	r28
	pop	r29
	ret

XEDly:
	ldi	  r25,0x20
	mov	  r1,r25
avr3B9:
	ldi	  r25,0xFF
avr3BA: 
	dec	  r25
	brne  avr3BA
	dec	  r1
	brne  avr3B9
	ret

XEStrt: 
	rcall	ClrPB0
	rcall	SetPB2
	rcall	SetPB0
	rcall	ClrPB2
	rcall	ClrPB0
	ret

XEStop: 
	rcall	ClrPB0
	rcall	ClrPB2
	rcall	SetPB0
	rcall	SetPB2
	rcall	ClrPB0
	ret

XEEIn:
	clr r0
	ldi r16,0x08
	rcall	SetPB2
	rcall	PB2In
avr3CF: 
	rcall SetPB0
	sbic  PINB,EE_SDA
	rjmp  avr3D5
	clc
	rol r0
	rjmp	avr3D7
avr3D5: sec
	rol r0
avr3D7: rcall	ClrPB0
	dec r16
	brne	avr3CF
	rcall	PB2Out
	ret

XEEOut: 
	ldi	  r16,0x08
	mov	  r0,r19
avr3DE: 
	clc
	rol	  r0
	brlo  avr3E4
	rcall ClrPB2
	rcall ClkPls
	rjmp  avr3E6
avr3E4: 
	rcall SetPB2
	rcall ClkPls
avr3E6: 
	dec	  r16
	brne  avr3DE
	ret

XE1Bit: rcall	SetPB2
	rcall	ClkPls
	ret

XE0Bit: 
	rcall	ClrPB2
	rcall	ClkPls
	ret

ClkPls: 
	rcall	SetPB0
	rcall	ClrPB0
	ret

SetPB2:
	sbi	 PORTB,EE_SDA
	rjmp PBExit

ClrPB2:
	cbi	 PORTB,EE_SDA
	rjmp PBExit

SetPB0:
	sbi	 PORTB,EE_SCL
	rjmp PBExit

ClrPB0: 
	cbi PORTB,EE_SCL
	rjmp	PBExit

PB2Out: 
	sbi	 DDRB,EE_SDA
	rjmp PBExit

PB2In:
	cbi	 DDRB,EE_SDA
	rjmp PBExit

PBExit:
	nop
	nop
	nop
	ret

; address r25:r24 
; result r25(=0):r24
fxeread:
	push	r29
	push	r28
	push	r16
	push	r1
	push	r0
	mov	r31,r25
	mov	r30,r24
; Start
	lds r28,last_addr
	lds r29,(last_addr)+1
	adiw r28,1
	cp r28,r24
	cpc r29,r25
	brne new_addr
	sts last_addr,r28
	sts (last_addr)+1,r29
	rcall fxeread_local_next
	rjmp fxeread_end
new_addr:
	sts last_addr,r24
	sts (last_addr)+1,r25
	rcall fxeread_local
fxeread_end:
; Done
	clr	r25
	mov	r24,r0
	pop	r0
	pop	r1
	pop	r16
	pop	r28
	pop	r29
	ret

fxeread_next:
	push	r29
	push	r28
	push	r16
	push	r1
	push	r0
; Start
	lds r28,last_addr
	lds r29,(last_addr)+1
	adiw r28,1
	sts last_addr,r28
	sts (last_addr)+1,r29
	rcall fxeread_local_next
; Done
	clr	r25
	mov	r24,r0
	pop	r0
	pop	r1
	pop	r16
	pop	r28
	pop	r29
	ret

fxeread_local:
	rcall	XE1Bit
	rcall	XEStop
	rcall	XEAddr
	rcall	XEStrt
	clc
	ldi	r19,0xA1
	rcall	XEEOut
	rcall	XE0Bit
	rcall	XEEIn
	ret

fxeread_local_next:
	rcall	XE0Bit
	rcall	XEEIn
	ret
