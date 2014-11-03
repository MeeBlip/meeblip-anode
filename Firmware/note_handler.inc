;-------------------------------------------------------------------------------------------------------------------
; Meeblip anode		Version 1.00
;
;
;-------------------------------------------------------------------------------------------------------------------
;			Note Handler  
;-------------------------------------------------------------------------------------------------------------------
;            
;			Processes note gating, portamento, note pitch
			
			;-----
            ;GATE:
            ;-----
		    lds	    r16, GATE
		    tst	    r16			        ; check GATE
		    brne	MLP_KEYON

;no key is pressed:

MLP_KEYOFF:
            ldi	    r16,4			    ;\

			; don't restart envelope release if it's already stopped.
			lds		r17, ENVPHASE
			tst		r17
			breq	MLP_NOTEON			; Don't put envelope 1 in release mode if it is already stopped	

		    sts	    ENVPHASE, r16		; "release"

;key is pressed:

MLP_KEYON:
            lds	    r16, GATEEDGE
		    tst	    r16		            ; Z=0 when key has just been pressed
		    breq	MLP_NOTEON

;key has just been pressed:
		    ldi	    r16, 0			    ;\
		    sts	    GATEEDGE, r16		;/ GATEEDGE = 0			
		    lds	    r16, PORTACNT		;\
		    tst	    r16			        ; \
		    breq	MLP_KEYON1		    ;  > if ( PORTACNT != 0 )
		    dec	    r16			        ; /    PORTACNT--
		    sts	    PORTACNT, r16		;/

MLP_KEYON1:

;envelope starts:	   
			ldi		r16, 1
		    sts	    ENVPHASE, r16		; attack

			;ldi	    R16, 0
		    ;sts	    ENV_FRAC_L, R16		;\
		    ;sts	    ENV_FRAC_H, R16		; > ENV = 0
		    ;sts	    ENV_INTEGR, R16		;/
 

; LFO starts when note triggered if SW_LFO_RETRIGGER = 0 (defaults to ON)


		    lds	    r16, PATCH_SWITCH1
		    sbrc 	r16, SW_LFO_RETRIGGER	
		    rjmp	MLP_NOTEON

		    ldi	    r16, 255		    ;\
		    ldi	    r17, 255		    ; > A = Amax
		    ldi	    r18, 127		    ;/
		    sts	    LFO_FRAC_L, r16		;\
		    sts	    LFO_FRAC_H, r17		; > store A
		    sts	    LFO_INTEGR, r18		;/
		    ldi	    r16, 1			    ;\
		    sts	    LFOPHASE, r16		;/ begin falling


MLP_NOTEON:
            ;-------------
            ;DCO A, DCO B:
            ;-------------
		    ldi	    r25, 0			    ;\
		    ldi	    r22, 0			    ; > r23,r22:r25 = note# 0..127
		    lds	    r23, LFONOTE		;/
		    cpi	    r23, 255
			brne	MLP_NLIM2
		    rjmp	MLP_VCOX

;note# limited to 36..96:

MLP_NLIM1:
            subi	r23, 12

MLP_NLIM2:
            cpi	    r23, 97
		    brsh	MLP_NLIM1
		    rjmp	MLP_NLIM4

MLP_NLIM3:
            subi	r23, 244

MLP_NLIM4:
            cpi	    r23, 36
		    brlo	MLP_NLIM3

;transpose 1 octave down:
		    subi	r23, 12			    ; n -= 12		Note range limited to 24..84

;portamento:
		    lds	    r25, NOTE_L		    ;\
		    lds	    r26, NOTE_H		    ; > r27,r26:r25 = nCurr
		    lds	    r27, NOTE_INTG		;/
		    lds	    r16, PORTACNT		;\
    		tst	    r16			        ; > jump when it's the first note
		    brne	MLP_PORTAWR	        ;/  (PORTACNT != 0)
		    lds	    r16, PORTAMENTO						
    		rcall	ADCTORATE
		    push    r22
		    push	r23
		    mov	    r22, r18		    ;\ r23:r22 = portamento rate
		    mov	    r23, r19		    ;/ 65535..27
		    ldi	    r16, 0
		    ldi	    r17, 0
		    lds	    r18, DELTAT_L
		    lds	    r19, DELTAT_H
		    ldi	    r20, 3
		    rcall	SHR32
		    rcall	MUL32X16		    ; r18,r17:r16 = nDelta
		    pop	    r23
		    pop	    r22
		    mov	    r19, r16		    ;\
		    mov	    r20, r17		    ; > r21,r20:r19 = nDelta
		    mov	    r21, r18		    ;/
		    lds	    r25, NOTE_L		    ;\
		    lds	    r26, NOTE_H		    ; > r27,r26:r25 = nCurr
		    lds	    r27, NOTE_INTG		;/
		    cp	    r22, r26		    ;\ nEnd - nCurr
		    cpc	    r23, r27		    ;/ Cy = (nEnd < nCurr)
		    brsh	MLP_PORTAADD

MLP_PORTAMIN:
            sub	    r25, r19			;\
		    sbc	    r26, r20			; > nCurr -= nDelta
		    sbc	    r27, r21			;/
		    cp	    r22, r26			;\ nEnd - nCurr;
		    cpc	    r23, r27		    ;/ Cy = (nEnd < nCurr)
		    brlo	MLP_PORTA1
		    rjmp	MLP_PORTAEND

MLP_PORTAADD:
            add	    r25, r19		    ;\
		    adc	    r26, r20		    ; > nCurr += nDelta
		    adc	    r27, r21		    ;/
		    cp	    r22, r26		    ;\ nEnd - nCurr;
		    cpc	    r23, r27		    ;/ Cy = (nEnd < nCurr)
		    brsh	MLP_PORTA1

MLP_PORTAEND:
            ldi	    r25, 0			    ;\
		    mov	    r26, r22			; > nCurr = nEnd
    		mov	    r27, r23			;/

MLP_PORTA1:
            mov	    r22, r26
		    mov	    r23, r27

MLP_PORTAWR:
        	sts	NOTE_L, r25
		    sts	    NOTE_H, r22
		    sts	    NOTE_INTG, r23

;pitch bender (-12..+12):
		    lds	    r16, MIDIPBEND_L	;\ r17,r16 = P.BEND
    		lds	    r17, MIDIPBEND_H	;/	-128,000..+127,996
		    ldi	    r18, 5			    ;\ r17,r16 = P.BEND/32
		    rcall	ASr16			    ;/	-4,000..+3,999
		    mov	    r18, r16		    ;\ r19,r18 = P.BEND/32
		    mov	    r19, r17		    ;/	-4,000..+3,999
		    add	    r16, r18		    ;\ r17,r16 = 2/32*P.BEND
		    adc	    r17, r19		    ;/	-8,000..+7,999
		    add	    r16, r18		    ;\ r17,r16 = 3/32*P.BEND
		    adc	    r17, r19		    ;/	-12,000..+11,999
		    add	    r22, r16		    ;\
		    adc	    r23, r17		    ;/ add P.BEND

MLP_PBX:
;for "DCF KBD TRACK":
		    sts	    PITCH, r23		    ; n = 0..108


;LFO modulation:
		    lds	    r16, PATCH_SWITCH1	; Check LFO destination bit. 
		    sbrc	r16, SW_LFO_DEST	; DCF is 0, DCO is 1
		    rjmp		MLP_VCOLFOX		    ; exit when LFO=DCF
		    lds	    r16, LFOVALUE		; r16 = LFO	    -128..+127
    		lds	    r17, LFOLEVEL		; r17 = LFO level	0..255

			lds		r18, LFOFREQ		; If LFO rate is = 0, turn LFO off
			cpi		r18, 0
			breq	LFO_OFF
			rjmp	LFO_NONLINEAR
LFO_OFF:
			ldi		r17, 0				; Set LFO level to zero if switch is off

LFO_NONLINEAR:
;nonlinear potentiometer function:
		    mov	    r18, r17		    ; r18 = LL
		    lsr	    r17			        ; r17 = LL/2
		    cpi	    r18, 128
		    brlo	MLP_OM1			    ; skip if LL = 0..127
		    subi	r17, 128		    ; r17 = 0,5*LL-128    -64..-1
		    add	    r17, r18		    ; r17 = 1,5*LL-128    +64..254

MLP_OM1:
			mulsu	r16, r17		; LFOVALUE*LFOLEVEL
			movw	r16, r0
		    ldi	    r18, 4			    ;\
		    rcall	ASr16			    ;/ r17,r16 = LFO*mod / 16
		    add	    r22, r16		    ;\
		    adc	    r23, r17		    ;/ add LFO to note #

;limiting to 0..108
		    tst	    r23
		    brpl	MLP_VCOLFO1
		    ldi	    r22, 0
		    ldi	    r23, 0
		    rjmp	MLP_VCOLFOX

MLP_VCOLFO1:
            cpi	    r23, 109
		    brlo	MLP_VCOLFOX
		    ldi	    r22, 0
		    ldi	    r23, 108

MLP_VCOLFOX:
            push	r22			        ;\ note# = 0..108
		    push	r23			        ;/ store for phase delta B

; determine the wavetable for osc A: note = 0..108

; Track which wavetable to use:
			mov		r25,	r23		; Store a copy of the note number in r25
			subi	r25,	13			; 13..108
			brcc	WTA_NOUFL
			clr		r25
WTA_NOUFL:
			lsr		r25
			lsr		r25			; (108-12-1)/8 = 11
			lsr		r25			; 0..11
			sts		WAVETABLE_A, r25	; Save wavetable 0..11 for lookup when generating oscillator

;phase delta A:
;octave A:
		    rcall	NOTERECALC		    ; r23,r22 = m12 (0,0..11,996),
						                ; r20 = n12 (0..11)
		    rcall	LOAD_DELTA		    ; r19:r18:r17:r16 = delta
		    rcall	SHL32			    ; r19:r18:r17:r16 = delta*(2^exp)

			  ; store delta
  			sts DELTAA_0,r17
  			sts DELTAA_1,r18
  			sts DELTAA_2,r19

;phase delta B:
		    pop	    r23			        ;\
		    pop	    r22			        ;/ n

;detune B:
		    lds	    r16, DETUNEB_FRAC	;\ r17,r16 = detuneB
		    lds	    r17, DETUNEB_INTG	;/ -128,000..+127,996
		    ldi	    r18, 4			    ;\ r17,r16 = detuneB / 16
    		rcall	ASR16			    ;/ -8,0000..+7,9998
		    add	    r22, r16		    ;\
		    adc	    r23, r17		    ;/

;octave B:
            lds	    r16, PATCH_SWITCH1	; octave B: 0=down, 1=up
		    
			; Shift down
			ldi		r25, 244			; store value in temp register
			sbrs	r16, SW_OSCB_OCT
		    add		r23, r25			; octave shift down

			; Shift osc b one octave down
			;sbrc	r16, SW_OSCB_OCT
			;subi	r23, 244		    ; n += 12

; determine the wavetable for osc B; r23: note = 0..108,
; Track which wavetable to use:
			mov		r25, r23			; Store a copy of the note number in r25
			subi	r25, 13				; 13..108
			brcc	WTB_NOUFL
			clr		r25
WTB_NOUFL:
			lsr		r25
			lsr		r25					; (108-12-1)/8 = 11
			lsr		r25					; 0..11
			sts		WAVETABLE_B, r25	; Save wavetable 0..15 for lookup when generating oscillator
			
		    rcall	NOTERECALC		    ; r23,r22 = m12 (0,0..11,996),
						                ; r20 = n12 (0..11)
		    rcall	LOAD_DELTA		    ; r19:r18:r17:r16 = delta
		    rcall	SHL32			    ; r19:r18:r17:r16 = delta*(2^exp)

			sts DELTAB_0,r17
  			sts DELTAB_1,r18
  			sts DELTAB_2,r19

MLP_VCOX: