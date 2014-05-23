;-------------------------------------------------------------------------------------------------------------------
; Meeblip anode		Version 1.00
;
;
;-------------------------------------------------------------------------------------------------------------------
;			Filter Modulation  
;-------------------------------------------------------------------------------------------------------------------
;            
;			Processes filter LFO modulation and enveloping

            ;----
            ;Filter
            ;----

	        ;LFO mod:
		    ldi	    r30, 0			    ;\
		    ldi	    r31, 0			    ;/ sum = 0

		    lds	    r16, PATCH_SWITCH1	; Check LFO destination bit. 
		    sbrs	r16, SW_LFO_DEST	; DCF is 0, DCO is 1
		    jmp		MLP_DCF0		    ; exit when LFO=DCO
		    lds	    r16, LFOVALUE		; r16 = LFO	    -128..+127
		    lds	    r17, LFOLEVEL		; r17 = DCF LFO MOD	0..255
			cpi		r17, 186
			brlo	lfo_freq
			ldi		r17, 186			; Limit LFOLEVEL to 186 if destination is VCF
LFO_FREQ:
			lds		r18, LFOFREQ		; If LFO rate is 0, turn LFO off
			cpi		r18, 0
			breq	LFO_OFF_DCF
			rjmp	MLP_DCF

LFO_OFF_DCF:
			ldi		r17, 0				; Set LFO level to zero if switch is off

MLP_DCF:
			mulsu	r16, r17
			mov		r30, r1
			ldi		r31, 0
			rol		r1					; r1.7 --> Cy (sign)
			sbc		r31, r31			; sign extension to r31

MLP_DCF0:

;ENV mod:
            lds	    r16, ENV_INTEGR	; Get the integer part of the envelope 
			lds		r17, VELOCITY_ENVMOD ; Use MIDI velocity * envmod 			
			mul		r16, r17
			movw 	r16,r0				; r17,r16 = AMPLITUDE ENVELOPE * ENVMOD		    
    		rol	    r16			        ; Cy = r16.7 (for rounding)
		    adc	    r30, r17
		    adc	    r31, ZERO

;KBD TRACK:
/*
		    lds	    r16, PITCH		    ; r16 = n (12/octave)	0..96
		    lsl	    r16			        ; r16 = 2*n (24/octave)	0..192
		    subi	r16, 96	        	; r16 = 2*(n-48) (24/octave)   -96..+96
		    ldi	    r17, 171
			
			mulsu	r16, r17
			movw	r16, r0
		    ldi	    r18, 0			    ;\
		    sbrc	r17, 7			    ; > r18 = sign extension
		    ldi	    r18, 255		    ;/  of r17
		    add	    r30, r17
		    adc	    r31, r18
*/

MLP_DCF3:
;CUTOFF:
		    lds	    r16, CUTOFF
		    clr	    r17
		    add	    r16, r30
    		adc	    r17, r31
		    tst	    r17
		    brpl	MLP_DCF1
		    ldi	    r16, 0
		    rjmp	MLP_DCF2

MLP_DCF1:
            breq	MLP_DCF2
		    ldi	    r16, 255

MLP_DCF2:
				
			; disable old/new value check. Just update.
			;lds		r17, OLD_CUTOFF
			;cp		r16, r17
			;breq	MLP_VCAENV

mlp_cutoff:
			;sts		OLD_CUTOFF, r16

			lsr	    r16			        ; 0..127
			mov		r17, r16			; Copy unscaled cutoff to r17
			ldi		r30, low( TAB_VCF)	;\
			ldi		r31, high( TAB_VCF)	;/ Z = &Tab
			rcall	TAB_BYTE		; r16 = 1.. 255
			sts		LPF_I, r16		; Store Lowpass F value (from exponential table)

mlp_output_cutoff:

			; Scale analog filter so it's slightly open at lowest knob level (avoids low end filter emphasis)
			mov		r16, r17		; Copy unscaled cutoff back to r16
			ldi		r30, low( TAB_VCF2)	;\
			ldi		r31, high( TAB_VCF2)	;/ Z = &Tab
			rcall	TAB_BYTE		; r16 = 1.. 255

			out     OCr2, r16		; load Cutoff value into PWM counter