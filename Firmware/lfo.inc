;-------------------------------------------------------------------------------------------------------------------
; Meeblip anode		Version 1.00
;
;
;-------------------------------------------------------------------------------------------------------------------
;			LFO Generation 
;-------------------------------------------------------------------------------------------------------------------
;
;			LFO	 : 	modulates oscillator pitch or filter cutoff
;			LFO2 :  sweeps PWM waveform duty cycle

;calculate dA:
			lds	    r16, LFOFREQ	    ;\

			lds	    r17, PATCH_SWITCH1	; Check LFO destination bit. 
		    sbrs	r17, SW_LFO_DEST	; DCF is 0, DCO is 1
			rjmp	MLP_LFO_NOLIMIT		; Don't limit knob if destination is oscillators
			cpi		r16, 220			; If VCF LFO rate > 220, limit it
			brlo	MLP_LFO_NOLIMIT
			ldi		r16, 220			; Set maximum rate for LFO when destination is the VCF

MLP_LFO_NOLIMIT:
		    com	    r16			        ;/ r16 = 255 - ADC0
		    rcall	ADCTORATE           ; r19:r18:r17:r16 = rate of rise/fall
		    lds	    r22, DELTAT_L		;\
    		lds	    r23, DELTAT_H		;/ r23:r22 = dT
		    rcall	MUL32X16		    ; r18:r17:r16 = dA
		    lds	    r19, LFO_FRAC_L
		    lds	    r20, LFO_FRAC_H
    		lds	    r21, LFO_INTEGR
		    subi    r21, 128
		    ldi	    r31, 0			    ; flag = 0
		    lds	    r30, LFOPHASE
		    tst	    r30
		    brne	MLP_LFOFALL

;rising phase:

MLP_LFORISE:
            lds	    r22, LFOTOP_0		;\
		    lds	    r23, LFOTOP_1		; > r24:r23:r22 = Amax
		    lds	    r24, LFOTOP_2		;/
		    add	    r19, r16		    ;\
    		adc	    r20, r17		    ; > A += dA
		    adc	    r21, r18		    ;/
		    brcs	MLP_LFOTOP
		    cp	    r19, r22		    ;\
		    cpc	    r20, r23		    ; > A - Amax
		    cpc	    r21, r24		    ;/
		    brlo	MLP_LFOX		    ; skip when A < Amax

;A reached top limit:

MLP_LFOTOP:
            mov	    r19, r22		    ;\
		    mov	    r20, r23		    ; > A = Amax
		    mov	    r21, r24		   	;/
		    ldi	    r30, 1			    ; begin of falling
		    ldi	    r31, 1			    ; flag = 1
		    rjmp	MLP_LFOX

;falling phase:

MLP_LFOFALL:
            lds	    r22, LFOBOTTOM_0	;\
		    lds	    r23, LFOBOTTOM_1	; > r24:r23:r22 = Amin
		    lds	    r24, LFOBOTTOM_2	;/
    		sub	    r19, r16		    ;\
		    sbc	    r20, r17		    ; > A -= dA
		    sbc	    r21, r18		    ;/
		    brcs	MLP_LFOBOTTOM
		    cp	    r22, r19		    ;\
		    cpc	    r23, r20		    ; > Amin - A
		    cpc 	r24, r21		    ;/
		    brlo	MLP_LFOX		    ; skip when A > Amin

;A reached bottom limit:

MLP_LFOBOTTOM:
            mov	    r19, r22		    ;\
		    mov	    r20, r23		    ; > A = Amin
		    mov	    r21, r24		    ;/
		    ldi	    r30, 0			    ; begin of rising
		    ldi	    r31, 1			    ; flag = 1

MLP_LFOX:
            sts	    LFOPHASE, r30
		    subi	r21, 128		    ; r21,r20:r19 = LFO tri wave
		    sts	    LFO_FRAC_L, r19		;\
		    sts	    LFO_FRAC_H, r20		; > store LFO value
    		sts	    LFO_INTEGR, r21		;/

;switch norm/rand:

;determine Amin i Amax:
		    ldi	    r16, 0			    ;\
		    ldi	    r17, 0			    ; > Amin when not LFO==tri
    		ldi	    r18, 0			    ;/  and not LFO==rand
		    
			; LFO random 
			lds	    r30, PATCH_SWITCH1
			sbrs 	r30, SW_LFO_RANDOM	; LFO random if switch is set

    		RJMP	MLP_LFOAWR
		    tst	    r31
    		breq	MLP_LFOAX
		    lds	    r16, SHIFTREG_0		;\
		    lds	    r17, SHIFTREG_1		; \ Amin = pseudo-random number
		    lds	    r18, SHIFTREG_2		; /	0,000..127,999
		    andi	r18, 0x7F		    ;/

MLP_LFOAWR:
            sts	    LFOBOTTOM_0, r16	;\
		    sts	    LFOBOTTOM_1, r17	; > store Amin
		    sts	    LFOBOTTOM_2, r18	;/
		    com	    r16			        ;\
		    com	    r17			        ; > Amax = 255,999 - Amin
		    com	    r18			        ;/	128,000..255,999
		    sts	    LFOTOP_0, r16		;\
		    sts	    LFOTOP_1, r17		; > store Amax
		    sts	    LFOTOP_2, r18		;/


MLP_LFOAX:
; Default to normal lfo behaviour (comment out to disable LFO random)
		    lds	    r16, PATCH_SWITCH1
		    sbrs 	r16, SW_LFO_RANDOM
		    rjmp	MLP_LFONORM
		    tst	    r31			        ; flag == 1 ?
		    breq	MLP_LFONWR		    ; jump when not
		    lds	    r21, SHIFTREG_2
		    rjmp	MLP_LFOWR


MLP_LFONORM:

;switch tri/squ:
/*
		    lds	    r16, PATCH_SWITCH1	;\ Z=0: triangle
			sbrs 	r16, SW_LFO_WAVE	;/ Z=1: square
    		rjmp	MLP_LFOWR
		    lsl	    r21			        ; Cy = (LFO < 0)
		    ldi	    r21, 127		    ;\
		    adc	    r21, ZERO		    ;/ r21 = -128 or +127
*/

MLP_LFOWR:
            sts	    LFOVALUE, r21

; Modulation wheel: Use highest value (Front panel or MIDI)
MLP_LFONWR:

		    lds	    r16, PANEL_LFOLEVEL
		    lds	    r17, MIDIMODWHEEL
		    cp	    r16, r17
    		brsh	MLP_LFOLWR
		    mov	    r16, r17		    ; MOD.WHEEL is greater

			lds	    r16, MIDIMODWHEEL


MLP_LFOLWR:
            sts	    LFOLEVEL, r16

MLP_LFOMWX:

            ;----
            ;LFO2 (Used to sweep PWM waveform)
            ;----

;calculate dA:	
			; Load fixed PWM sweep rate
			ldi		r16, 180

		    rcall	ADCTORATE           ; r19:r18:r17:r16 = rate of rise/fall
		    lds	    r22, DELTAT_L		;\
    		lds	    r23, DELTAT_H		;/ r23:r22 = dT
		    rcall	MUL32X16		    ; r18:r17:r16 = dA
		    lds	    r19, LFO2_FRAC_L
		    lds	    r20, LFO2_FRAC_H
    		lds	    r21, LFO2_INTEGR
		    subi    r21, 128
		    ldi	    r31, 0			    ; flag = 0
		    lds	    r30, LFO2PHASE
		    tst	    r30
		    brne	MLP_LFO2FALL

;rising phase:

MLP_LFO2RISE:
            lds	    r22, LFO2TOP_0		;\
		    lds	    r23, LFO2TOP_1		; > r24:r23:r22 = Amax
		    lds	    r24, LFO2TOP_2		;/
		    add	    r19, r16		    ;\
    		adc	    r20, r17		    ; > A += dA
		    adc	    r21, r18		    ;/
		    brcs	MLP_LFO2TOP
		    cp	    r19, r22		    ;\
		    cpc	    r20, r23		    ; > A - Amax
		    cpc	    r21, r24		    ;/
		    brlo	MLP_LFO2X		    ; skip when A < Amax

;A reached top limit:

MLP_LFO2TOP:
            mov	    r19, r22		    ;\
		    mov	    r20, r23		    ; > A = Amax
		    mov	    r21, r24		   	;/
		    ldi	    r30, 1			    ; begin of falling
		    ldi	    r31, 1			    ; flag = 1
		    rjmp	MLP_LFO2X

;falling phase:

MLP_LFO2FALL:
            lds	    r22, LFO2BOTTOM_0	;\
		    lds	    r23, LFO2BOTTOM_1	; > r24:r23:r22 = Amin
		    lds	    r24, LFO2BOTTOM_2	;/
    		sub	    r19, r16		    ;\
		    sbc	    r20, r17		    ; > A -= dA
		    sbc	    r21, r18		    ;/
		    brcs	MLP_LFO2BOTTOM
		    cp	    r22, r19		    ;\
		    cpc	    r23, r20		    ; > Amin - A
		    cpc 	r24, r21		    ;/
		    brlo	MLP_LFO2X		    ; skip when A > Amin

;A reached bottom limit:

MLP_LFO2BOTTOM:
            mov	    r19, r22		    ;\
		    mov	    r20, r23		    ; > A = Amin
		    mov	    r21, r24		    ;/
		    ldi	    r30, 0			    ; begin of rising
		    ldi	    r31, 1			    ; flag = 1

MLP_LFO2X:
            sts	    LFO2PHASE, r30
		    subi	r21, 128		    ; r21,r20:r19 = LFO2 tri wave
		    sts	    LFO2_FRAC_L, r19	;\
		    sts	    LFO2_FRAC_H, r20	; > store LFO2 value
    		sts	    LFO2_INTEGR, r21	;/

			subi	r21, $80			; remove sign
            sts	    PULSE_WIDTH, r21	; Update pulse width value