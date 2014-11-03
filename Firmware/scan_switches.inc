;-------------------------------------------------------------------------------------------------------------------
; Meeblip anode		Version 1.00
;
;
;-------------------------------------------------------------------------------------------------------------------
;
;---------------------
; scan panel switches:
;---------------------
;begin:

		    in	    r16, TCNT1L		    ;\
		    in	    r17, TCNT1H		    ;/ r17:r16 = t
		    lds	    r18, TPREV_KBD_L	;\
		    lds	    r19, TPREV_KBD_H	;/ r19:r18 = t0
		    sub	    r16, r18			;\
		    sbc	    r17, r19			;/ r17:r16 = t - t0
		    subi	r16, LOW(KBDSCAN)	;\
		    sbci	r17, HIGH(KBDSCAN)	;/ r17:r16 = (t-t0) - 100ms
		    brsh	MLP_SCAN		    ;\
		    rjmp	MLP_SKIPSCAN		;/ skip scanning if (t-t0) < 100ms

MLP_SCAN:


; Read panel switches
						
			in		r16, PINC	; Read PORTC pin values (front panel switches)	
		    lds	    r17, SWITCH1
			lds		r18, PATCH_SWITCH2

; MIDI Channel momentary switch (PORT C0)
			bst		r16, 0
			bld		r18, SW_MIDI		; Copy Port C0 value into MIDI Channel set switch

; LFO Destination 	PATCH_SWITCH1, SW_LFO_DEST (PORT C4)
			bst		r16, 4
			bld		r17, SW_LFO_DEST	; Copy PORT C4 into SW_LFO_DEST
			
; PWM_SWEEP 		PATCH_SWITCH1, SW_PWM_SWEEP (PORT C5)
			bst		r16, 5
			bld		r17, SW_PWM_SWEEP	; Copy PORT C5 into SW_PWM_SWEEP
			
; Octave			PATCH_SWITCH2, SW_OSCB_OCT (PORT C6)
			bst		r16, 6
			bld		r17, SW_OSCB_OCT	; Copy PORT C6 into SW_OSCB_OCT

; Envelope Sustain	PATCH_SWITCH2, SW_SUSTAIN (PORT C7)
			bst		r16, 7
			bld		r17, SW_SUSTAIN	; Copy PORT C7 into SW_SUSTAIN
			
		    sts	    SWITCH1, r17 ; Store switch settings				
		    sts	    PATCH_SWITCH2, r18 ; Store MIDI switch setting
			
; Make sure we capture switch settings at power-up, so we know when they're changed.
			
			lds		r19, POWER_UP		; Is this the first time through this code since synth was turned on?
			sbrs	r19, 0				
			rjmp	MLP_SWITCH
			sts		OLD_SWITCH1, r17	; Yes: make OLD_SWITCHx = SWITCHx (so we know when they've been flipped)
			sts	    PATCH_SWITCH1, r17	; Read starup patch switch values from the front panel
			clr		r19					
			sts		POWER_UP, r19		; Clear the POWER_UP flag so we don't reinitialize


; ---------------------------------------------------------------------------------------------------------------
; Only update switch value if the physical switch has moved, to avoid overwriting MIDI CC parameter changes
; 
; ---------------------------------------------------------------------------------------------------------------
MLP_SWITCH:
			; Compare SWITCH1 to OLD_SWITCH1. Skip if unchanged.

			lds		r16, SWITCH1
			lds		r17, OLD_SWITCH1	
			cp		r16, r17		
			breq	MLP_SWITCH_EXIT			; Switch 1 unchanged, so leave as is.
			lds		r18, PATCH_SWITCH1	

			; Compare bits in OLD_SWITCH1 and SWITCH1. If different, copy SWITCH1 bit to PATCH_SWITCH1 bit.
			; Perform an exclusive OR on r17 and r16. Changed bits are flagged as 1's.
			eor		r17, r16	

;MLP_BIT0:			
			sbrs	r17, 0				
			rjmp	MLP_BIT1			; Exit if bit is not set
			bst		r16, 0
			bld		r18, 0				; copy bit from SWITCH1 to PATCH_SWITCH1
MLP_BIT1:

			sbrs	r17, 1				
			rjmp	MLP_BIT2	
			bst		r16, 1
			bld		r18, 1		
							
MLP_BIT2:
			sbrs	r17, 2				
			rjmp	MLP_BIT3			
			bst		r16, 2
			bld		r18, 2	
					
MLP_BIT3:
			sbrs	r17, 3				
			rjmp	MLP_BIT4			
			bst		r16, 3
			bld		r18, 3
		
MLP_BIT4:
			sbrs	r17, 4				
			rjmp	MLP_BIT5			
			bst		r16, 4
			bld		r18, 4				

MLP_BIT5:
			sbrs	r17, 5				
			rjmp	MLP_BIT6			
			bst		r16, 5
			bld		r18, 5				

MLP_BIT6:
			sbrs	r17, 6				
			rjmp	MLP_BIT7			
			bst		r16, 6
			bld		r18, 6					

MLP_BIT7:
			sbrs	r17, 7				
			rjmp	MLP_PATCH1SAVE			
			bst		r16, 7
			bld		r18, 7		


MLP_PATCH1SAVE:	

			sts		PATCH_SWITCH1, r18		; Store changes	to patch
			sts		OLD_SWITCH1, r16		; Keep a copy of panel switches so we know if things change next time
			
MLP_SWITCH_EXIT:			


MLP_SKIPSCAN:

; Check MIDI Set switch
			lds		r16, BUTTON_STATUS
			cpi		r16, 3						; If button has been pressed, allow LED to flash
			breq	control_exit

			lds		r21, PATCH_SWITCH2
			sbrc	r21, SW_MIDI				; 0(pressed)/1(not pressed)
			rjmp	CONTROL_EXIT				; button not pressed

			; blink LED if MIDI button has been pressed
			ldi		r16, 1
			sts		BUTTON_STATUS, r16	; Set control status to MIDI, turn on control timer
			ldi		r16, 255
			sts		LED_TIMER, r16
			ldi		r16, 96
			sts		LED_TIMER2, r16
			clr		r16
			rcall	CLEAR_KNOB_STATUS	; Snap to current knob positons, so we know if they're moved to set a new channel

control_exit:
			lds		r16, BUTTON_STATUS
			tst		r16
			breq	MLP_LED_END			; button hasn't been pressed, so skip

			lds		r17, LED_TIMER
			lds		r18, LED_TIMER2
			sub		r17, r16			; r16 contains the button flash rate	
			sbc		r18, ZERO

			tst		r18		; hit zero?
			breq	hit_zero
			sts		LED_TIMER, r17
			sts		LED_TIMER2, r18

			; figure out if LED should be on or off. 
			sbrs	r18, 1
			cbi		PORTD, 1			; LED off if bit 1 in r18 is zero
			sbrc	r18, 1
			sbi		PORTD, 1			; LED on if bit 1 is one

			rjmp	MLP_LED_END
hit_zero:	
			sbi	    PORTD, 1		    ; LED on
			ldi		r16, 0
			sts		BUTTON_STATUS, r16	; Turn off button

MLP_LED_END: