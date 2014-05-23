;-------------------------------------------------------------------------------------------------------------------
; Meeblip anode		Version 1.10
;
;
;-------------------------------------------------------------------------------------------------------------------
;			I/O subroutines
;-------------------------------------------------------------------------------------------------------------------
;
;-----------------------------------------------------------------------------
;A/D conversion (start)
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	    r18 = channel #	        0..7
;Out:	-
;Used:	-
;-----------------------------------------------------------------------------
ADC_START:
            out	    ADMUX, r18	        ; set multiplexer
		    sbi	    ADCSRA, 6	        ; ADSC=1
		    ret

;-----------------------------------------------------------------------------
;A/D conversion (end)
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	-
;Out:	    r16 = result		            0..255
;Used:	    SREG,r17
;-----------------------------------------------------------------------------
ADC_END:
ADCE_LOOP:
            sbis	ADCSRA, 4 	        ;\
		    rjmp	ADCE_LOOP	        ;/ wait for ADIF==1
		    sbi	    ADCSRA, 4 		    ; clear ADIF
		    in	    r16, ADCL	        ;\
		    in	    r17, ADCH	        ;/ r17:r16 = 000000Dd:dddddddd
		    lsr	    r17		            ;\
		    ror	    r16		            ;/ r17:r16 = 0000000D:dddddddd
		    lsr	    r17		            ;\
		    ror	    r16		            ;/ r16 = Dddddddd
		    ret

;=============================================================================
;			arithmetic subroutines
;=============================================================================

;-----------------------------------------------------------------------------
; 16 bit arithmetical shift right (division by 2^n)
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	    r17:r16 = x
;	        r18 = n (shift count)		0..16
;Out:	    r17:r16 = x asr n
;Used:	    SREG
;-----------------------------------------------------------------------------
ASr16:
            tst	    r18
		    breq	ASr16_EXIT
		    push	r18

ASr16_LOOP:
            asr	    r17		            ;\
		    ror	    r16		            ;/ r17,r16 = r17,r16 asr 1
		    dec	    r18
		    brne	ASr16_LOOP
		    pop	    r18

ASr16_EXIT:
            ret

;-----------------------------------------------------------------------------
; 32 bit logical shift right
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	    r19:r18:r17:r16 = x
;	        r20 = n (shift count)
;Out:	    r19:r18:r17:r16 = x >> n
;Used:	    SREG
;-----------------------------------------------------------------------------
SHr32:
            tst	    r20
		    breq	SHr32_EXIT
		    push	r20

SHr32_LOOP:
            lsr	    r19
		    ror	    r18
		    ror	    r17
		    ror	    r16
		    dec	    r20
		    brne	SHr32_LOOP
		    pop	    r20

SHr32_EXIT:
            ret

;-----------------------------------------------------------------------------
; 32 bit logical shift left
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	    r19:r18:r17:r16 = x
;	        r20 = n (shift count)
;Out:	    r19:r18:r17:r16 = x << n
;Used:	    SREG
;-----------------------------------------------------------------------------
SHL32:
            tst	    r20
		    breq	SHL32_EXIT
		    push	r20

SHL32_LOOP:
            lsl	    r16
		    rol	    r17
		    rol	    r18
		    rol	    r19
		    dec	    r20
		    brne	SHL32_LOOP
		    pop	    r20

SHL32_EXIT:
            ret

;-----------------------------------------------------------------------------
;32 bit x 16 bit multiplication (unsigned)
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	r19:r18:r17:r16 = x		0..2^32-1
;	r23:r22 = y			0,yyyyyyyyyyyyyyyy  0..0,9999847
;Out:	r19:r18:r17:r16 = x * y	0..2^32-1
;Used:	SREG,r20-r29
;-----------------------------------------------------------------------------
MUL32X16:

; multiply with high multiplier x
		mul		r19, r23		; ax
		movw	r29:r28, r1:r0
		mul		r18, r23		; bx
		movw	r21:r20, r1:r0
		mul		r17, r23		; cx
		movw	r27:r26, r1:r0
		mul		r16, r23		; dx
		mov		r25, r0
		add		r26, r1
		adc		r27, r20
		adc		r28, r21
		adc		r29, ZERO
; multiply with low multiplier y
		mul		r19, r22		; ay
		movw	r21:r20, r1:r0
		mul		r17, r22		; cy
		add		r25, r0
		adc		r26, r1
		adc		r27, r20
		adc		r28, r21
		adc		r29, ZERO
		mul		r18, r22		; by
		movw	r21:r20, r1:r0
		mul		r16, r22		; dy
		mov		r24, r0
		add		r25, r1
		adc		r26, r20
		adc		r27, r21
		adc		r28, ZERO
		adc		r29, ZERO

		mov		r16,r26			;\
		mov		r17,r27			; \
		mov		r18,r28			; / x * y
		mov		r19,r29			;/

		ret
;-----------------------------------------------------------------------------
; Load 32 bit phase value from ROM
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	    r30 = index
;Out:	    r19:r18:r17:r16 = value
;Used:	    SREG,r0,r30,r31
;-----------------------------------------------------------------------------
LOAD_32BIT:
            lsl	    r30			        ; r30 *= 2
		    ldi	    r31, 0
		    adiw	r30, DELTA_C	    ; Z = ROM address
		    add	    r30, r30
    		adc	    r31, r31
			lpm		r16, z+
			lpm		r17, z+
			lpm		r18, z+
			lpm		r19, z+
		    ret

;-----------------------------------------------------------------------------
; Load phase delta from ROM
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	    r23,r22 = indexs = 0,0..12,0 = n,octave
;Out:	    r19:r18:r17:r16 = delta
;Used:	    SREG,r0,r21,r24-r31
;-----------------------------------------------------------------------------
LOAD_DELTA:
            push	r22
		    push	r23
		    mov	    r30, r23
    		rcall	LOAD_32BIT
		    mov	    r24, r16
		    mov	    r25, r17
		    mov	    r26, r18
		    mov	    r27, r19		    ; r27-r24 = delta[n]
		    mov	    r30, r23
		    inc	    r30
		    rcall	LOAD_32BIT
		    sub	    r16, r24
		    sbc	    r17, r25
		    sbc	    r18, r26
		    sbc	    r19, r27
		    push	r24
		    push	r25
		    push	r26
		    push	r27
		    mov	    r23, r22
		    ldi	    r22, 0
		    push	r20
		    rcall	MUL32X16
		    pop	    r20
		    pop	    r27
		    pop	    r26
		    pop	    r25
		    pop	    r24
    		add	    r16, r24
		    adc	    r17, r25
    		adc	    r18, r26
		    adc	    r19, r27
		    pop	    r23
		    pop	    r22
		    ret

;-----------------------------------------------------------------------------
;note number recalculation
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	    r23 = n	                    0..139 = m12 + 12*n12
;Out:	    r23 = m12                   0..11
;	        r20 = n12                   0..11
;Used:	    SREG
;-----------------------------------------------------------------------------
NOTERECALC:
            ldi	r20,0			        ; n12 = 0
		    rjmp	NRC_2

NRC_1:
            subi	r23, 12			    ; m12 -= 12
		    inc	    r20			        ; n12++

NRC_2:
            cpi	    r23, 12
		    brsh	NRC_1			    ; repeat while m12 >= 12
		    ret

;-----------------------------------------------------------------------------
;read a byte from a table
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	    r16 = i		                0..255
;	        r31:r30 = &Tab
;Out:	    r16 = Tab[i]	                0..255
;Used:	    SREG,r30,r31
;-----------------------------------------------------------------------------
TAB_BYTE:
            add	    r30, r30			;\
		    adc	    r31, r31		    ;/ Z = 2 * &Tab
		    add	    r30, r16
		    adc	    r31, ZERO
		    lpm		r16, z
		    ret

;-----------------------------------------------------------------------------
;read a word from a table
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	    r16 = i			            0..255
;	        r31:r30 = &Tab
;Out:	    r19:r18 = Tab[i]            0..65535
;Used:	    SREG,r0,r30,r31
;-----------------------------------------------------------------------------
TAB_WORD:
            add	    r30, r16
		    adc	    r31, ZERO
		    add	    r30, r30		    ;\
		    adc	    r31, r31		    ;/ Z = 2 * &Tab
		    lpm		r18, z+				; LSByte
			lpm		r19, z				; MSByte
		    ret

;-----------------------------------------------------------------------------
;"time" --> "rate" conversion
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	    r16 = time			        0..255
;Out:	    r19:r18:r17:r16 = rate		0x001B0000..0xFFFF0000
;Used:	    SREG,r0,r30,r31
;-----------------------------------------------------------------------------
ADCTORATE:
            lsr	    r16
		    lsr	    r16
		    lsr	    r16			        ;0..31
			ldi		r30, low( TIMETORATE)
			ldi		r31, high(TIMETORATE)
		    rcall	TAB_WORD		    ;r19:r18 = rate
		    clr	    r16
		    clr	    r17
		    ret

;-----------------------------------------------------------------------------
;conversion of the "detune B" potentiometer function
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	    r16 = x		                0..255
;Out:	    r17,r16 = y	                0,000..255,996
;Used:	    SREG,r18-r30
;-----------------------------------------------------------------------------
NONLINPOT:
            ldi	    r22, 0
		    mov	    r23, r16
    		cpi	    r23, 112
		    brlo	NLP_I
		    cpi	    r23, 144
		    brlo	NLP_II
		    rjmp	NLP_III

; 62.5 kHz calcs.  

NLP_I:
            ldi	    r16, 0			    ;\  r18,r17:r16 = m =
		    ldi	    r17, 32			    ; > = 126/112 =
		    ldi	    r18, 1			    ;/  = 1.125
    		ldi	    r30, 0			    ;\ r31,r30 = n =
		    ldi	    r31, 0			    ;/ = 0,0
		    rjmp	NLP_CONT

NLP_II:
            ldi	    r16, 8			    ;\  r18,r17:r16 = m =
		    ldi	    r17, 33			    ; > = (130-126)/(143-112) =
    		ldi	    r18, 0			    ;/  = 0.129032258
		    ldi	    r30, 140		    ;\ r31,r30 = n =
		    ldi	    r31, 111		    ;/ = 126 - m*112 = 111.5483871
		    rjmp	NLP_CONT

NLP_III:
            ldi	    r16, 183		    ;\  r18,r17:r16 = m =
		    ldi	    r17, 29			    ; > = (255-130)/(255-143) =
		    ldi	    r18, 1			    ;/  = 1.116071429
    		ldi	    r30, 103		    ;\ r31,r30 = n =
		    ldi	    r31, 226		    ;/ 255 - m*255 = -29.59821429

NLP_CONT:
            ldi	    r19, 0
		    rcall	MUL32X16
		    add	    r16, r30
		    adc	    r17, r31
		    ret

;-----------------------------------------------------------------------------
; Write byte to eeprom memory
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	    r16 	= value		                0..255
;			r18:r17 = eeprom memory address
;Used:	    r16, r17, r18
;-----------------------------------------------------------------------------
EEPROM_write:
										; Wait for completion of previous write
			sbic 	EECR,EEWE
			rjmp 	EEPROM_write
			in		temp_SREG, SREG		; save SREG
			cli							; disable interrupts during timed eeprom sequence
			out 	EEARH, r18 
			out 	EEARL, r17			; single byte offset from WRITE_OFFSET
			out 	EEDR,  r16			; Write data (r16) to data register
			sbi 	EECR,EEMWE			; Write logical one to EEMWE
			sbi 	EECR,EEWE			; Start eeprom write by setting EEWE
			out		SREG, temp_SREG		; restore SREG (restarts interrupts if enabled)
			ret									

;-----------------------------------------------------------------------------
; Read byte from eeprom memory
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	    r18:r17 = eeprom memory address
;Out:		r16 	= value		                0..255
;Used:	    r16, r17, r18
;-----------------------------------------------------------------------------
EEPROM_read:
										
			sbic 	EECR,EEWE			; Wait for completion of previous write
			rjmp 	EEPROM_read
			in		temp_SREG, SREG		; save SREG
			cli							; disable interrupts during timed eeprom sequence
			out 	EEARH, r18			; Set up address (r18:r17) in address register
			out 	EEARL, r17
			sbi 	EECR, EERE			; Start eeprom read by writing EERE
			in 		r16, EEDR			; Read data from data register
			out		SREG, temp_SREG 	; restore SREG (restarts interrupts if enabled)
			ret

;-----------------------------------------------------------------------------
; Set new MIDI channel, reset LED flash
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; In:		r16 = New MIDI Channel
; Out:		NONE
; Used:		r16, r17, r18
;-----------------------------------------------------------------------------
SET_MIDI_CHANNEL:

			sts	    SETMIDICHANNEL, r16		
			ldi		r18, $03
			ldi		r17, $FF				; Set eeprom address to $03FF (last byte in memory)
			rcall   EEPROM_WRITE			; Write r16 to eeprom for recall next time we power on

			; Don't simply reset LED. Instead, reset the timer and set the Status to 2 (successfully set)
			;sbi	    PORTD, 1		        ; Clear control button parameters and leave the LED on
			;ldi		r16, 0					;
			;sts		BUTTON_STATUS, r16		; Clear midi set button

			; blink LED double fast if midi channel has been set. 
			ldi		r16, 3
			sts		BUTTON_STATUS, r16	; Set control status to channel set success, turn on control timer
			ldi		r16, 255
			sts		LED_TIMER, r16
			ldi		r16, 48
			sts		LED_TIMER2, r16
			ret

;-----------------------------------------------------------------------------
; Clear knob status
; Set knob status to 'unmoved' and save current knob positions
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;In:	    --
;Out:		--
;Used:	    r16
;-----------------------------------------------------------------------------

CLEAR_KNOB_STATUS:						;	set knob status to 'unmoved' and save current knob positions
			clr		r16
			sts		KNOB0_STATUS, r16	; 
			sts		KNOB1_STATUS, r16	; 
			sts		KNOB2_STATUS, r16	; 
			sts		KNOB3_STATUS, r16	; 
			sts		KNOB4_STATUS, r16	; 
			sts		KNOB5_STATUS, r16	; 
			sts		KNOB6_STATUS, r16	; 
			sts		KNOB7_STATUS, r16	; 
			lds	    r16, ADC_0			; Save current pot positions for future comparison
			sts	    OLD_ADC_0,r16
			lds	    r16, ADC_1			 
			sts	    OLD_ADC_1,r16
			lds	    r16, ADC_2			 
			sts	    OLD_ADC_2,r16
			lds	    r16, ADC_3			 
			sts	    OLD_ADC_3,r16
			lds	    r16, ADC_4			 
			sts	    OLD_ADC_4,r16
			lds	    r16, ADC_5			 
			sts	    OLD_ADC_5,r16
			lds	    r16, ADC_6			 
			sts	    OLD_ADC_6,r16
			lds	    r16, ADC_7			 
			sts	    OLD_ADC_7,r16	
			ret


;-----------------------------------------------------------------------------
; Scan a pot and update its value if it has been moved
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
; In: r16 - the new pot value
;	  r20 - the current conversion channel (0..7) 
; Out: r17 - 1 if the pot has changed, 0 otherwise
; Used: r16-r20, r28, r29, SREG
;-----------------------------------------------------------------------------
POT_SCAN:
			ldi		r28, low(KNOB0_STATUS)
			ldi		r29, high(KNOB0_STATUS)
			add		r28, r20		
			adc		r29, ZERO
		    ld	    r18, Y			    ; load KNOBN_STATUS value into r18
			
			sbrc	r18, 0				; Check bit 0
			rjmp	LOAD_ADC			; KNOBN_STATUS is set, so just update parameter
			mov		r19, r16
			
			ldi		r28, low(OLD_ADC_0)
			ldi		r29, high(OLD_ADC_0)
			add		r28, r20		
			adc		r29, ZERO
		    ld	    r17, Y			    ; load OLD_ADC_N value into r17
			sub		r19, r17
			brpl	DEAD_CHECK
			neg		r19		
DEAD_CHECK:
			cpi		r19, 5				 
			brlo	NO_CHANGE			; Skip ahead if pot change is < the deadzone limit
			sbr 	r18,1				; Update knob status bit and continue -- pot moved
			ldi		r28, low(KNOB0_STATUS)
			ldi		r29, high(KNOB0_STATUS)
			add		r28, r20		;
			adc		r29, ZERO
		    st      Y, r18			    ; save updated KNOBN_STATUS
			rjmp 	LOAD_ADC			; 

NO_CHANGE:
			ldi		r17, 0			    ; flag pot unchanged
			ret

LOAD_ADC:	
			ldi		r17, 1				; flag pot changed
			ret 					