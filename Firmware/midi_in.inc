;-------------------------------------------------------------------------------------------------------------------
; Meeblip anode		Version 1.10
;
;
;-------------------------------------------------------------------------------------------------------------------
;		M I D I 
;-------------------------------------------------------------------------------------------------------------------
;
; UART receiver (MIDI IN)
;

UART_RXC:

            push	r16
		    in	    r16, SREG	        ;\
		    push	r16			        ;/ push SREG
			push	r17 

		    in	    r16, UDR	        ; read received byte in r16
		    cbi	    UCR, 7		        ; RXCIE=0 (disable UART interrupts)


; Exit UART interrupt immediately if received MIDI value => $F0
			cpi		r16, $F0			; ignore active sensing, midi clocks, sysex
			brlo	INTRX_PROCESS
			ldi		r16, 0
			sts	    MIDIPHASE, r16		; MIDIPHASE = 0
			;sei				            ; enable other interrupts
			rjmp	INTRX_EXIT			; quit UART loop as quickly as possible

INTRX_PROCESS:
; end
			;sei				            ; enable other interrupts
		    tst	    r16		            ;\ jump when
		    brpl	INTRX_DATA		    ;/ r16.7 == 0 (MIDI data byte)

;MIDI status byte (1xxxxxxx):
		    mov	    r17, r16
		    andi	r17, 0xF0
		    cpi	    r17, 0x80
		    breq	INTRX_ACCEPT	    ; 8x note off
		    cpi	    r17, 0x90
		    breq	INTRX_ACCEPT	    ; 9x note on
		    cpi	    r17, 0xB0
		    breq	INTRX_ACCEPT	    ; Bx control change
		    cpi	    r17, 0xE0
		    breq	INTRX_ACCEPT	    ; Ex pitch bend
		    ldi	    r17, 0		        ;\
		    sts	    MIDIPHASE, r17	    ;/ MIDIPHASE = 0
		    rjmp	INTRX_EXIT		    ; Ax polyphonic aftertouch
						                ; Cx program change
						                ; Dx channel aftertouch
						                ; Fx system

INTRX_ACCEPT:
            sts	    MIDIPHASE, r17	    ; phase = 80 90 B0 E0
		    andi	r16, 0x0F		    ;\
		    inc	    r16			        ; > store MIDI channel 1..16
		    sts	    MIDICHANNEL, r16	;/
		    lds	    r17, SETMIDICHANNEL	;0 for OMNI or 1..15
		    tst	    r17
		    breq	INTRX_ACPT_X		; end when OMNI
		    cp	    r17, r16			; compare set channel to the incoming channel
		    breq	INTRX_ACPT_X		; end when right channel
		    ldi	    r17, 0			    ;\ otherwise:
		    sts	    MIDIPHASE, r17		;/ MIDIPHASE = 0 (no data service)

INTRX_ACPT_X:
            rjmp	INTRX_EXIT

;MIDI data byte (0xxxxxxx):
INTRX_DATA:
            lds	    r17, MIDIPHASE
		    cpi	    r17, 0x80		    ;\
		    breq	INTRX_NOFF1		    ; \
		    cpi	    r17, 0x81		    ; / note off
		    breq	INTRX_NOFF2		    ;/
		    rjmp	INTRX_NOTEON

INTRX_NOFF1:
            inc	    r17			        ;\
		    sts	    MIDIPHASE, r17	    ;/ MIDIPHASE = 0x81
		    sts	    MIDIDATA0, r16	    ; MIDIDATA0 = d
		    rjmp	INTRX_EXIT

INTRX_NOFF2:
            dec	    r17			        ;\
		    sts	    MIDIPHASE, r17	    ;/ MIDIPHASE = 0x80
		    rjmp	INTRXNON2_OFF

;9x note on:
INTRX_NOTEON:
            cpi	    r17, 0x90		    ;\
		    breq	INTRX_NON1		    ; \
		    cpi	    r17, 0x91		    ; / note on
		    breq	INTRX_NON2		    ;/
		    rjmp	INTRX_CTRL

INTRX_NON1:
            inc     r17			        ;\
		    sts	    MIDIPHASE, r17	    ;/ MIDIPHASE = 0x91
		    sts	    MIDIDATA0, r16	    ; MIDIDATA0 = d
		    rjmp	INTRX_EXIT

INTRX_NON2:
            dec	    r17			        ;\
		    sts	    MIDIPHASE, r17	    ;/ MIDIPHASE = 0x90
		    tst	    r16			        ;\
		    brne	INTRXNON2_ON	    ;/ jump when velocity != 0

;turn note off:
INTRXNON2_OFF:
            lds	    r16, MIDIDATA0
		    lds	    r17, MIDINOTEPREV
		    cp	    r16, r17
		    brne	INTRXNON2_OFF1
		    ldi	    r17, 255		    ;\ remove previous note
		    sts	    MIDINOTEPREV, r17	;/ from buffer

INTRXNON2_OFF1:
            lds	    r17, MIDINOTE
		    cp	    r16, r17		    ;\
		    brne	INTRXNON2_OFF3	    ;/ exit when not the same note
		    lds	    r17, MIDINOTEPREV
		    cpi	    r17, 255
		    breq	INTRXNON2_OFF2
		    sts	    MIDINOTE, r17		; previous note is valid
			sts		LFONOTE, r17		; Keep a copy for LFO
		    ldi	    r17, 255		    ;\ remove previous note
		    sts	    MIDINOTEPREV, r17	;/ from buffer

INTRXNON2_OFF3:
            rjmp	INTRX_EXIT

INTRXNON2_OFF2:
            ldi	    r17, 255		    ;\ remove last note
		    sts	    MIDINOTE, r17		;/
		    ldi	    r17, 0			    ;\
		    sts	    GATE, r17		    ;/ GATE = 0
			sbi	    PORTD, 1		    ; LED on
		    rjmp	INTRX_EXIT

;turn note on:
INTRXNON2_ON:
            sts	    MIDIVELOCITY, r16	; store velocity
		    lds	    r17, MIDINOTE		;\ move previous note
		    sts	    MIDINOTEPREV, r17	;/ into buffer
		    lds	    r17, MIDIDATA0		;\
		    sts	    MIDINOTE, r17		;/ MIDINOTE = note#
			sts		LFONOTE, r17		; Store note for LFO
		    ldi	    r17, 1
		    sts	    GATE, r17		    ; GATE = 1
		    sts	    GATEEDGE, r17		; GATEEDGE = 1
		    
			cbi	    PORTD, 1		    ; LED off
		    rjmp	INTRX_EXIT

;Bx control change:
INTRX_CTRL:
            cpi	    r17, 0xB0		    ;\
		    breq	INTRX_CC1		    ; \
		    cpi	    r17, 0xB1		    ; / control change
		    breq	INTRX_CC2		    ;/
		    rjmp	INTRX_PBEND

INTRX_CC1:
            inc     r17			        ;\
		    sts	    MIDIPHASE, r17		;/ MIDIPHASE = 0xB1
		    sts	    MIDIDATA0, r16		; MIDIDATA0 = controller#
		    rjmp	INTRX_EXIT

INTRX_CC2:
            dec     r17			        ;\
		    sts	    MIDIPHASE, r17		;/ MIDIPHASE = 0xB0
		    lds	    r17, MIDIDATA0

;Store MIDI CC in table
			push 	r26					; store contents of r27 and r26 on stack
			push	r27

			cpi		r17, $30			; Just save a controller # < $30
			brlo	INTRX_GOSAVE

			cpi		r17, $40			; save, update old knob value and status
			brlo	INTRX_KNOB

			cpi		r17, $50			; save, update old switch value and status
			brlo	INTRX_SW

INTRX_GOSAVE:
			rjmp	INTRX_SAVE			; Save all other controller # > $50

INTRX_KNOB:
			; save the value in the MIDI table
			ldi 	r26,low(MIDICC)			
  			ldi 	r27,high(MIDICC)
  			add 	r26,r17
  			adc 	r27,zero
  			lsl 	r16					; shift MIDI data to 0..254 to match knob value
  			st 		x,r16				; store in MIDI CC table

			; Get ADC_X and write it into OLD_ADC_X
			subi	r17, $30			; reduce to 0..15
			cbr		r17, $f8			; Clear highest 5 bits, leaving knob 0..7		

			ldi 	r26,low(ADC_0)			
  			ldi 	r27,high(ADC_0)
  			add 	r26,r17
  			adc 	r27,zero
			ld		r16, x				; Fetch ADC_X into r16

			ldi 	r26,low(OLD_ADC_0)			
  			ldi 	r27,high(OLD_ADC_0)
  			add 	r26,r17
  			adc 	r27,zero
			st		x, r16				; Store ADC_X in OLD_ADC_X

			; Clear KNOBX_STATUS (knob not moved)
			ldi 	r26,low(KNOB0_STATUS)			
  			ldi 	r27,high(KNOB0_STATUS)
  			add 	r26,r17
  			adc 	r27,zero
			ldi		r17, 0
			st		x, r17				; Clear KNOBX_STATUS
			rjmp	INTRX_CCEND

INTRX_SW:
		subi	r17, $40			; MIDI CC # --> switch offset 0..15
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		
INTRX_SWITCH1:		
		
		cbr		r17, $f8			; Clear highest 5 bits, leaving switch 0..7
		lds		r26, PATCH_SWITCH1
		bst	    r16, 6 		; load MSB of MIDI CC value into SREG T bit	
		
		cpi		r17, 0 
		brne	INTRX_SW1		
		bld		r26, 0			; Set bit in PATCH_SWITCH1
		rjmp	INTRX_SWEXIT
INTRX_SW1:
		cpi		r17, 1 
		brne	INTRX_SW2		
		bld		r26, 1			; Set bit in PATCH_SWITCH1
		rjmp	INTRX_SWEXIT
INTRX_SW2:
		cpi		r17, 2 
		brne	INTRX_SW3		
		bld		r26, 2			; Set bit in PATCH_SWITCH1
		rjmp	INTRX_SWEXIT
INTRX_SW3:
		cpi		r17, 3 
		brne	INTRX_SW4		
		bld		r26, 3			; Set bit in PATCH_SWITCH1
		rjmp	INTRX_SWEXIT
INTRX_SW4:
		cpi		r17, 4 
		brne	INTRX_SW5		
		bld		r26, 4			; Set bit in PATCH_SWITCH1
		rjmp	INTRX_SWEXIT
INTRX_SW5:
		cpi		r17, 5 
		brne	INTRX_SW6		
		bld		r26, 5			; Set bit in PATCH_SWITCH1
		rjmp	INTRX_SWEXIT
INTRX_SW6:
		cpi		r17, 6 
		brne	INTRX_SW7		
		bld		r26, 6			; Set bit in PATCH_SWITCH1
		rjmp	INTRX_SWEXIT
INTRX_SW7:			
		bld		r26, 7			; Set bit in PATCH_SWITCH1

INTRX_SWEXIT:					; Finished switch scan, store updated switch bytes
		sts		PATCH_SWITCH1, r26
		rjmp	INTRX_CCEND

INTRX_SAVE:
			ldi 	r26,low(MIDICC)			
  			ldi 	r27,high(MIDICC)
  			add 	r26,r17
  			adc 	r27,zero
  			lsl 	r16					; shift MIDI data to 0..254 to match knob value
  			st 		x,r16				; store in MIDI CC table

INTRX_CCEND:
			pop		r27					; reload old contents of r27 and r 26
			pop		r26
		    rjmp	INTRX_EXIT

;Ex pitch bender:
INTRX_PBEND:
            cpi	    r17, 0xE0		    ;\
		    breq	INTRX_PB1		    ; \
		    cpi	    r17, 0xE1		    ; / pitch bend
		    breq	INTRX_PB2		    ;/
			rjmp	INTRX_EXIT

INTRX_PB1:
            inc     r17			        ;\
		    sts	    MIDIPHASE, r17		;/ MIDIPHASE = 0xE1
		    sts	    MIDIDATA0, r16		; MIDIDATA0 = dFine	0..127
		    rjmp	INTRX_EXIT

INTRX_PB2:
            dec	    r17			        ;\
		    sts	    MIDIPHASE, r17		;/ MIDIPHASE = 0xE0
		    lds	    r17,MIDIDATA0		;\
		    lsl	    r17			        ;/ r17 = dFine*2	0..254
		    lsl	    r17			        ;\ r16,r17 = P.B.data
		    rol	    r16			        ;/ 0..255,996
		    subi	r16, 128		    ; r16,r17 = -128,000..+127,996
		    sts	    MIDIPBEND_L, r17	;\
		    sts	    MIDIPBEND_H, r16	;/ store P.BEND value
		    rjmp	INTRX_EXIT

INTRX_EXIT:
            pop	    r17
		    pop	    r16			        ;\
		    out	    SREG, r16		    ;/ pop SREG
		    pop	    r16
		    sbi	    UCR, 7			    ; RXCIE=1
		    reti