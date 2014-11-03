;-------------------------------------------------------------------------------------------------------------------
; Meeblip anode		Version 1.10
;
;
;-------------------------------------------------------------------------------------------------------------------
;		S A M P L E     G E N E R A T I O N     L O O P 
;-------------------------------------------------------------------------------------------------------------------
; Timer 2 compare interrupt (sampling)
;
; This is where sound is generated. This interrupt is called 40,000 times per second 
; to calculate a single 16-bit value for audio output. There are 500 instruction cycles 
; (16MHZ/40,000) between samples, and these have to be shared between this routine and the 
; main program loop that scans controls, receives MIDI commands and calculates envelope, 
; LFO, and DCA/DCF levels.
;
; If you use too many clock cycles here there won't be sufficient time left over for
; general housekeeping tasks. The result will be sluggish and lost notes, weird timing and sadness.
;-------------------------------------------------------------------------------------------------------------------
;

; Push contents of registers onto the stack
;
TIM0_CMP:
		    push	r16
		    in	    r16, SREG		    ;\
    		push	r16			        ;/ push SREG
		    push	r17
			push    r18
			push	r19
			push	r20
			push    r21
			push	r22
			push	r23
			push	r30
			push	r31
  			push r0
  			push r1

;-------------------------------------------------------------------------------------------------------------------
;
; Oscillator A & B 
;
; This design uses direct frequency synthesis to generate a ramp wave. A three-byte counter (= phase) is being
; incremented by a value which is proportional to the sound frequency (= phase delta). The
; increment takes place every sampling period. The most significant byte of the counter is a sawtooth ramp. 
; This is either used as a pointer to a 256 byte wavetable or for direct waveform synthesis.
; Each oscillator has its own phase and phase delta registers. The contents of each phase delta 
; register depends on the frequency being generated:
;
;                   PHASE DELTA = 2 ^ 24 * Freq / SamplingFreq
;
; where:
;       SamplingFreq = 40000 Hz
;       Freq = 440 * 2 ^ ((n - 69 + d) / 12)
;       where in turn:
;           n = MIDI note number. Range limited to 36 to 96 (5 octaves)
;           d = transpose/detune (in halftones)
;
;-------------------------------------------------------------------------------------------------------------------



; Output first byte of envelope LEVEL to the SPI DAC

			; Output DCA Level
			; Pull DCA chip select line low to enable clock + data
			cbi		PORTD, 3
			; Shift most significant byte into SPI register 
			
			lds		r30, LEVEL_H
			out		SPDR, r30	
				

; Pulse wave or wavetable version?
			lds		r21, FIRMWARE
			sbrc	r21, 0	
			jmp		WAVE_DCOA		; If FIRMWARE = 1, execute alternate firmware (wavetables)

;Calculate DCO A							

CALC_DCOA:
		    mov	    r17, PHASEA_2				; sawtooth ramp for OSCA

			lds		r21, PATCH_SWITCH1
			sbrs	r21, SW_PWM_SWEEP  ; 0/1 (pulse/PWM)	
			rjmp	PULSE_OSCA					; On = PWM

			lds		r23, PULSE_WIDTH			; pulse width from LFO
			rjmp	PULSEA


PULSE_OSCA:

			lds		r23, PULSE_KNOB_LIMITED		; get range limited pulse width from knob
			rjmp	PULSEA

SAW_OSCA:
			lds		r22, WAVETABLE_A			; Load wavetable offset 0..11
			
			cpi		r22, 5					
			brlo	RAW_SAWA					; Use raw wave if we're playing wavetable 0..4

			; anti-aliased sawtooth
DCOA_SAW:
												; Offset to the correct wavetable in r22, based on note number (0..11)
			ldi		ZL, low (2*INV_SAW0)		; Load low part of byte address into ZL
			ldi		ZH, high(2*INV_SAW0)		; Load high part of byte address into ZH
			add		ZL, r17						; Offset the wavetable by the ramp phase (i)
			adc		ZH, r22						; Wavetable 0..11
			lpm									;	Load wave(i) into r0
			mov		r17, r0						;	Copy into DCO A
RAW_SAWA:
			rjmp	EXIT_PULSEA					; Raw sawtooth. We already have a sawtooth ramp loaded into OSCA (r17)		

PULSEA:											
			lds		r22, WAVETABLE_A			; Load wavetable offset 0..11

			cpi		r22, 5					
			brlo	RAW_PULSE					; Use raw wave if we're playing wavetable 0..4

				; r17 phase
				; r18 temp
				; r23 pulse width
				; r22 wavetable
				; get sample a into r17
			ldi		ZL, low (2*INV_SAW0)		; Load low part of byte address into ZL
			ldi		ZH, high(2*INV_SAW0)		; Load high part of byte address into ZH
			add		ZL, r17						; Offset the wavetable by the ramp phase (i)
			adc		ZH, r22						; Wavetable 0..11
			lpm									; Load wave(i) into r0

	; get sample b out of phase into r18
			mov		r18, r23					; Grab a copy of the pulse width
			add		r18, r17					; Add phase offset for second table (pulse width + original sample)
			mov		r17, r0						; store previous sample in r17
			ldi		ZL, low (2*INV_SAW0)		; Load low part of byte address into ZL
			ldi		ZH, high(2*INV_SAW0)		; Load high part of byte address into ZH
			add		ZL, r18						; Add phase offset for second table.
			adc		ZH, r22						; Wavetable 0..15
			lpm									; Load wave(i) into r0

	; subtract wave a-b		
	; first part b > a, second part b < a
			clr		r18
			sub		r17, r0			
			sbc		r18, ZERO
			add		r17, r23					; add offset (pulse width)
			adc		r18, ZERO
			brge	PULSE_BOUND_CHECK			; non-negative result, so no need to limit the value
			ldi		r17, 0
			ldi		r18, 0						; value was negative, so force to zero
PULSE_BOUND_CHECK:
			tst		r18							; Check if we're larger than 255
			breq	PWM_EXIT					; no need to limit upper bound
			ldi		r17, $FF
PWM_EXIT:
			rjmp	EXIT_PULSEA

RAW_PULSE:
			; Raw Pulse wave generated on the fly. Aliases like crazy (calc'd only when anti-alias switch is OFF)
			cp		r17, r23			
			brlo	PULSE_ZERO	
			ldi		r17, $00					; Set pulse to bottom
			rjmp	EXIT_PULSEA
PULSE_ZERO:
			ldi		r17, $FF					; Set pulse to top
EXIT_PULSEA:
			subi    r17, $80					; sign oscillator A


			; Output second byte of DCA Level on SPI
			lds		r30, LEVEL_L
			out		SPDR, r30

CALC_DCOB:
		    mov	    r16, PHASEB_2				; sawtooth ramp for OSCA

			lds	    r23, PATCH_SWITCH1
		    sbrc 	r23, SW_OSCB_WAVE			; SW_OSCB_WAVE: 1 = sawtooth, 0 = pulse 	
		    rjmp	SAW_OSCB

PULSE_OSCB:

			lds		r23, PULSE_KNOB_LIMITED		; get range limited pulse width from knob
			rjmp	PULSEB

SAW_OSCB:
			lds		r22, WAVETABLE_B			; Load wavetable offset 0..11

			cpi		r22, 5					
			brlo	RAW_SAWB					; Use raw wave if we're playing wavetable 0..4

			; anti-aliased sawtooth
DCOB_SAW:
												; Offset to the correct wavetable in r22, based on note number (0..11)
			ldi		ZL, low (2*INV_SAW0)		; Load low part of byte address into ZL
			ldi		ZH, high(2*INV_SAW0)		; Load high part of byte address into ZH
			add		ZL, r16						; Offset the wavetable by the ramp phase (i)
			adc		ZH, r22						; Wavetable 0..11
			lpm									;	Load wave(i) into r0
			mov		r16, r0						;	Copy into DCO B

RAW_SAWB:			 

			rjmp	EXIT_PULSEB					; Raw sawtooth. We already have a sawtooth ramp loaded into OSCA (r17)


PULSEB:																					
			lds		r22, WAVETABLE_B			; Load wavetable offset 0..11
			
			cpi		r22, 5					
			brlo	RAW_PULSEB					; Use raw wave if we're playing wavetable 0..4

				; r16 phase
				; r18 temp
				; r23 pulse width
				; r22 wavetable
				; get sample a into r17
			ldi		ZL, low (2*INV_SAW0)		; Load low part of byte address into ZL
			ldi		ZH, high(2*INV_SAW0)		; Load high part of byte address into ZH
			add		ZL, r16						; Offset the wavetable by the ramp phase (i)
			adc		ZH, r22						; Wavetable 0..15
			lpm									; Load wave(i) into r0

	; get sample b out of phase into r18
			mov		r18, r23					; Grab a copy of the pulse width
			add		r18, r16					; Add phase offset for second table (pulse width + original sample)
			mov		r16, r0						; store previous sample in r17
			ldi		ZL, low (2*INV_SAW0)		; Load low part of byte address into ZL
			ldi		ZH, high(2*INV_SAW0)		; Load high part of byte address into ZH
			add		ZL, r18						; Add phase offset for second table.
			adc		ZH, r22						; Wavetable 0..15
			lpm									; Load wave(i) into r0

	; subtract wave a-b		
	; first part b > a, second part b < a
			clr		r18
			sub		r16, r0			
			sbc		r18, ZERO
			add		r16, r23					; add offset (pulse width)
			adc		r18, ZERO
			brge	PULSE_BOUND_CHECKB			; non-negative result, so no need to limit the value
			ldi		r16, 0
			ldi		r18, 0						; value was negative, so force to zero
PULSE_BOUND_CHECKB:
			tst		r18							; Check if we're larger than 255
			breq	PWM_EXITB					; no need to limit upper bound
			ldi		r16, $FF
PWM_EXITB:
			rjmp	EXIT_PULSEB

RAW_PULSEB:
			; Raw Pulse wave generated on the fly. Aliases like crazy (calc'd only when we're playing octave 0..4)
			cp		r16, r23			
			brlo	PULSE_ZEROB	
			ldi		r16, $00					; Set pulse to bottom
			rjmp	EXIT_PULSEB
PULSE_ZEROB:
			ldi		r16, $FF					; Set pulse to top
EXIT_PULSEB:
			subi    r16, $80					; sign oscillator B

			;ldi		r16, $80	; silence osc b (test)
		
;-------------------------------------------------------------------------------------------------------------------
; Sum Oscillators
;
; Combines DCOA (in r17) and DCOB (in r16) waves to produce a 16-bit signed result in HDAC:LDAC (r17:r16)


; Mixer out = (A*x + B*(1-x))/4   x=0..1
			ldi		r22, 135			; Set DCO A level = 135  
			mulsu	r17, r22			; signed DCO A wave * level
			movw	r30, r0				; store value in temp register
			ldi		r22, 119			; Set DCO B level = 120
			mulsu	r16, r22			; signed DCO B wave * level
			add		r30, r0
			adc		r31, r1				; sum scaled waves
			movw	r16, r30			; copy signed output into r16

			jmp		PRE_DCF

;-------------------------------------------------------------------------------------------------------------------
; Wavetable Oscillators
;
WAVE_DCOA:
		    mov	    r16, PHASEA_2				; sawtooth ramp for OSCA

			lds		r22, WAVETABLE				; Wavetable 0..15
			ldi		r17, 8						; Number of tables in a bank

			lds		r21, PATCH_SWITCH1
			sbrc	r21, SW_PWM_SWEEP  ; 0/1 (pulse/PWM)	
			add		r22, r17					; Increment wavetable to second bank (waves 8-15)

			add		r22, r22					; Multiply WAVETABLE by 2 because each table is 512 bytes (256 x 16-bit samples)

			clr		r17							; zero r17
			add		r16, r16					; Multiply ramp by 2 (512 byte table) and store in r17:r16
			adc		r17, ZERO					; carry
																

DCOA_WAVES:
												
			ldi		ZL, low (2*WAVES)		; Load low part of byte address into ZL
			ldi		ZH, high(2*WAVES)		; Load high part of byte address into ZH
			add		ZL, r16					; Offset the wavetable by the ramp phase (i)
			adc		ZH, r17

			add		ZH, r22					; Offset wavetable 0..15 x 2 (each sample is 2 bytes)

			lpm		r18, Z+						; Copy half of the sample into sample high byte and increment pointer 
			lpm		r19, Z						; Copy other half of sample into sample low byte 

									
;Calculate DCO B							

WAVE_DCOB:
		    mov	    r16, PHASEB_2				; sawtooth ramp for OSCA

			clr		r17							; zero r17
			add		r16, r16					; Multiply ramp by 2 (512 byte table) and store in r17:r16
			adc		r17, ZERO					; carry


DCOB_WAVES:
												
			ldi		ZL, low (2*WAVES)		; Load low part of byte address into ZL
			ldi		ZH, high(2*WAVES)		; Load high part of byte address into ZH
			add		ZL, r16						; Offset the wavetable by the ramp phase (i)
			adc		ZH, r17

			add		ZH, r22					; Offset wavetable 0..15 x 2 (each sample is 2 bytes)

			lpm		r16, Z+						; Copy half of the sample into sample high byte and increment pointer 
			lpm		r17, Z						; Copy other half of sample into sample low byte 


			NOP NOP NOP NOP
			NOP NOP NOP NOP

			; Output second byte of DCA Level on SPI
			lds		r30, LEVEL_L
			out		SPDR, r30



			; Shift both oscillators one bit right to make sure we don't overflow the sum.
			asr	    r17		            ;\
			ror	    r16		            ;/ r17:r16 = r17:r16 asr 1

			asr	    r19		            ;\
		    ror	    r18		            ;/ r19:r18 = r19:r18 asr 1

			add		r16, r18
			adc		r17, r19			; Sum the oscillators


; Finish loading cutoff or envelope level, pull CS high. 

			; Make sure SPI has enough time to complete.
			NOP	NOP	NOP	NOP
			NOP	NOP	NOP	NOP
			NOP	NOP	NOP	NOP
			NOP	NOP	NOP	NOP
					
			NOP	NOP	NOP	NOP
			NOP	NOP	NOP	NOP
			NOP	NOP	NOP	NOP
			NOP	NOP	NOP	NOP

PRE_DCF:
; Finish loading cutoff or envelope level, pull CS high. 
			sbi		PORTD, 3	; CS (DAC)

			
;-------------------------------------------------------------------------------------------------------------------
; Digitally Controlled Filter
;-------------------------------------------------------------------------------------------------------------------
; 
; A 2-pole resonant low pass filter:
;
; a += f * ((in - a) + q * (a - b));
; b += f * (a - b); 
;
; Input 16-Bit signed HDAC:LDAC (r17:r16), already scaled to minimize clipping (reduced to 25% of full code).
;-------------------------------------------------------------------------------------------------------------------
;

DCF:
			; rotate right to make headroom for resonance.  

            asr	    r17		            ;\
		    ror	    r16		            ;/ r17:r16 = r17:r16 asr 1
			
.INCLUDE "digital_filter.inc"	
				
				
;-------------------------------------------------------------------------------------------------------------------
; Digitally Controlled Amplifier
;
; Multiply the output waveform by the 8-bit value in LEVEL.
; r17:r16 - output from filter 16b signed
; r18     - output from DCA envelope generator
;-------------------------------------------------------------------------------------------------------------------
;

DCA:

			lds		r18, level
			cpi		r18, 255
			breq	T2_AEXIT	    ; don't multiply when LEVEL==255, use
			mulsu	r17, r18		; multiply samples high byte
			movw	r30, r0
			mul		r16, r18		; multipliy samples low byte
			add		r30, r1
			adc		r31, ZERO		

T2_AEXIT:


;-------------------------------------------------------------------------------------------------------------------
; Output Sample
;
; Write the 16-bit signed output of the DCA to the DAC.
;-------------------------------------------------------------------------------------------------------------------
;

;write sample (r31:r30) to DAC:

; Control bit mask xxxx0011
			ldi		r17, 0b00000011 ; Outputs audio signal through DACA, through the Vref pin on DACB. 

; Pull DAC chip select line low to enable clock + data
			cbi		PORTD, 3

; Shift MSB data word in r17:r16:r30 left 4 bits, so it's packed 0011 dddd dddd dddd
            subi	r31, 128		    ; U2 --> PB
			mov		r16, r31

; 12-bit audio (requires MCP4922 DAC for full 12-bit resolution)
			lsl		r30		; shift lsb 1 bit left
			rol		r16		; rotate through r16
			rol		r17		; and into r17
			lsl		r30		; shift 2 bits left
			rol		r16		; rotate through r16
			rol		r17		; and into r17
			lsl		r30		; shift 3 bits left
			rol		r16		; rotate through r16
			rol		r17		; and into r17
			lsl		r30		; shift 4 bits left
			rol		r16		; rotate through r16
			rol		r17		; and into r17


; Shift most significant byte into SPI register 
			out		SPDR, r17

; Shift LSB into r17 and do something else for >16 clock cycles while the previous byte shifts in
			mov		r17, r16

; Increment Oscillator A & B phase

  			ldi 	r30, low(DELTAA_0)
  			ldi 	r31, high(DELTAA_0)
  			ld 		r16,z+
  			add 	PHASEA_0, r16
  			ld 		r16,z+
  			adc 	PHASEA_1, r16
  			ld 		r16,z+
  			adc 	PHASEA_2, r16
  			ld 		r16,z+
  			add 	PHASEB_0, r16
  			ld 		r16,z+
  			adc 	PHASEB_1, r16
  			ld 		r16, z+
  			adc 	PHASEB_2,r16

			; Shift least significant byte into SPI register 
			out		SPDR, r17


;-------------------------------------------------------------------------------------------------------------------
; End of Sample Interrupt
;
; Pop register values off stack and return to our regularly scheduled programming.
;-------------------------------------------------------------------------------------------------------------------
; 

			pop 	r1
  			pop 	r0
			pop		r31
			pop		r30
			pop		r23
			pop		r22
			pop		r21
			pop		r20
			pop		r19
			pop     r18 
		    pop	    r17

; wait 16 cycles for byte to load into DAC, then load both channels and stop SPI clock
			;  load DAC
			cbi		PORTD, 4
			;  Pull DAC chip select line high to disable
			sbi		PORTD, 3
			sbi		PORTD, 4

		    pop	    r16		            ;\
		    out	    SREG, r16	        ;/ pop SREG
		    pop	    r16
		    reti