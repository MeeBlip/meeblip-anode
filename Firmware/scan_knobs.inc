;-------------------------------------------------------------------------------------------------------------------
; Meeblip anode		Version 1.00
;
;
;-------------------------------------------------------------------------------------------------------------------
;
;--------------------
;read potentiometers:
;--------------------

		    rcall	ADC_END			    ; r16 = AD(i)
		    lds	    r18, ADC_CHAN		;\
			sts		PREV_ADC_CHAN, r18   ; keep track of which ADC channel we're processing. 
			ldi		r28, low(ADC_0)
			ldi		r29, high(ADC_0)
			add		r28, r18
			adc		r29, ZERO
		    st	    Y, r16			    ; AD(i) --> ADC_i

;next channel:
		    inc	    r18
		    andi	r18, 0x07
		    sts	    ADC_CHAN,r18
		    rcall	ADC_START	        ; start conversion of next channel
			
; ------------------------------------------------------------------------------------------------------------------------
; Read potentiometer values
; ------------------------------------------------------------------------------------------------------------------------
;
			lds		r20, PREV_ADC_CHAN	; Only process the most recently scanned pot, store ADC channel 0..7 in r20

CHECK_0:
			cpi		r20, 0				; Update knob 0, LFO Depth?
			brne	CHECK_1
			rcall	POT_SCAN			; If so, check if parameter should be updated with pot value in r16
			tst		r17					
			breq	EXIT_POTS			; Skip update if pot hasn't been updated
		    sts	    LFOLEVEL,r16		
			sts		PANEL_LFOLEVEL, r16	

			; Check if MIDI set switch has been pressed. If so, set MIDI CHANNEL
			lds		r16, BUTTON_STATUS
			tst		r16
			breq	EXIT_POTS				; button hasn't been pressed, so skip
			cpi		r16, 3
			breq	EXIT_POTS
			ldi		r16, 4					; MIDI CHANNEL 4
			rcall	SET_MIDI_CHANNEL

			rjmp	END_POTS

CHECK_1:
			cpi		r20, 1				; Update knob 1, LFO rate?
			brne	CHECK_2
			rcall	POT_SCAN			; If so, check if parameter should be updated with pot value in r16
			tst		r17					
			breq	EXIT_POTS
			sts	    LFOFREQ,r16

			; Check if MIDI set switch has been pressed. If so, set MIDI CHANNEL
			lds		r16, BUTTON_STATUS
			tst		r16
			breq	EXIT_POTS				; button hasn't been pressed, so skip
			cpi		r16, 3
			breq	EXIT_POTS
			ldi		r16, 3					; MIDI CHANNEL 3
			rcall	SET_MIDI_CHANNEL
			rjmp	END_POTS

CHECK_2:
			cpi		r20, 2				; Update knob 2, Detune?
			brne	CHECK_3
			rcall	POT_SCAN			; If so, check if parameter should be updated with pot value in r16
			tst		r17					
			breq	EXIT_CHECK2			; Skip update if pot hasn't been updated
			sts		OSC_DETUNE, r16

			; Check if MIDI set switch has been pressed. If so, set MIDI CHANNEL
			lds		r16, BUTTON_STATUS
			tst		r16
			breq	EXIT_CHECK2				; button hasn't been pressed, so skip
			cpi		r16, 3
			breq	EXIT_CHECK2
			ldi		r16, 2					; MIDI CHANNEL 2
			rcall	SET_MIDI_CHANNEL

EXIT_CHECK2:
			lds		r16, OSC_DETUNE		; grab the patch value, just in case it hasn't been updated
		    rcall	NONLINPOT		    ; AD6.1 --> DCO B detune with non-linear knob (center is tuned)
		    subi	r17, 128		     
		    sts	    DETUNEB_FRAC, r16	; Value -128.000..+127.996
		    sts	    DETUNEB_INTG, r17	
EXIT_POTS:
			rjmp	END_POTS

CHECK_3:
			cpi		r20, 3				; Update knob 3, Shape (PWM)?
			brne	CHECK_4	
			rcall	POT_SCAN			; If so, check if parameter should be updated with pot value in r16
			tst		r17					
			breq	EXIT_CHECK3			; Skip update if pot hasn't been updated
			sts     PULSE_KNOB, r16
			lsr		r16					; 0..127
			lsr		r16					; 0..63
			lsr		r16					; 0..31
			lsr		r16					; 0..15
			lsr		r16					; 0..8
			sts		WAVETABLE, r16		; Knob value selects one of 8 wavetables  

			; Check if MIDI set switch has been pressed. If so, set MIDI CHANNEL
			lds		r16, BUTTON_STATUS
			tst		r16
			breq	EXIT_CHECK3				; button hasn't been pressed, so skip
			cpi		r16, 3
			breq	EXIT_CHECK3
			ldi		r16, 1					; MIDI CHANNEL 1
			rcall	SET_MIDI_CHANNEL

EXIT_CHECK3:
			lds		r16, PULSE_KNOB		; grab the patch value, just in case it hasn't been updated
			lsr		r16					; Divide it by two (we only need 0-50% pulse)
										; Limit to 10-127
			cpi		r16, 10
			brlo	CHECK_3LIMIT
			rjmp	CHECK_3A
CHECK_3LIMIT:
			ldi		r16, 10				; Limit minimum Pulse width to 10
CHECK_3A:
			sts		PULSE_KNOB_LIMITED, r16
			rjmp	END_POTS

CHECK_4:
			cpi		r20, 4				; Update knob 4, Filter Cutoff?
			brne	CHECK_5		
			rcall	POT_SCAN			; If so, check if parameter should be updated with pot value in r16
			tst		r17					
			breq	END_POTS				; Skip update if pot hasn't been updated
			sts	    CUTOFF,r16					
			rjmp	END_POTS

CHECK_5:	
			cpi		r20, 5				; Update knob 5, Decay?
			brne	CHECK_6	
			rcall	POT_SCAN			; If so, check if parameter should be updated with pot value in r16	
			tst		r17					
			breq	CHECK_5B				; Skip update if pot hasn't been updated		
			sts	    KNOB_AMP_DECAY, r16	

CHECK_5B:
			lds	    r16, KNOB_AMP_DECAY 		; Grab the value, in case it hasn't been updated (IE: MIDI CC parameter update)
			lds		r19, PATCH_SWITCH1
			sbrs	r19, SW_SUSTAIN				
			rjmp	DCF_SUSTAIN_OFF		    ; Skip if sustain is off...
			ldi		r19, 255					; Sustain is on, so...
			sts		SUSTAINLEVEL, r19			; Set sustain to maximum
			sts		DECAYTIME, r19				; Set decay to maximum
			sts		RELEASETIME, r16			; Set release time to value of decay knob
			rjmp	END_POTS

DCF_SUSTAIN_OFF:
			ldi		r19, 0						; Sustain is off, so...		
			sts		SUSTAINLEVEL, r19			; Set sustain to minimum
			sts		DECAYTIME, r16				; Set decay time to value of decay knob
			sts		RELEASETIME, r16			; Set release time to value of decay knob
			rjmp	END_POTS			
			
CHECK_6:
			cpi		r20, 6					; Update knob 6, Attack?
			brne	END_POTS
			rcall	POT_SCAN				; If so, check if parameter should be updated with pot value in r16	
			tst		r17					
			breq	END_POTS				; Skip update if pot hasn't been updated		
			sts		KNOB_AMP_ATTACK, r16    

END_POTS: