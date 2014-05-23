;-------------------------------------------------------------------------------------------------------------------
; Meeblip anode		Version 1.00
;
;
;-------------------------------------------------------------------------------------------------------------------
;			DCA Output  
;-------------------------------------------------------------------------------------------------------------------
;            
;			Look up DCA value and output the DCA level amount to the second channel of the DAC chip. 


            ;---------------
            ;sound level:
            ;---------------

MLP_VCAENV:
            lds	    r16, ENV_INTEGR		; 
			ldi		r30, low( TAB_VCA)	;\
			ldi		r31, high( TAB_VCA)	;/ Z = &Tab
			rcall	TAB_BYTE			; r0 = 2..255 (load LEVEL into r16)
MLP_VCAOK:
			sts		LEVEL,r16

			; Non-zero starting level for DAC B to minimize envelope pop

			ldi		r30, 96		; Add 96 to level
			add		r16, r30	
			brcc	MLP_DCAOUT  ; if value is less than 255, output it
			ldi		r16, 255	; if carry bit is set, force level to 255


MLP_DCAOUT:

			; DCA LEVEL is in r16. Now pack it in two bytes for the DCA. 
			; Control bit mask 0001 0001
			;ldi		r17, 0b00010001 	; Write data to P0

			; Control bit mask xxxx1011
			ldi		r17, 0b00001011 		; outputs signal through DACB 

; Shift MSB data word in r17:r16 left 4 bits, so it's packed 1011 dddd dddd xxxx

; Use DACB for DCA LEVEL amount
			lsl	    r16
		    rol	    r17		; shifted 1 bit left
			lsl	    r16
		    rol	    r17		; shifted 2 bits left
			lsl	    r16
		    rol	    r17		; shifted 3 bits left
			lsl	    r16
		    rol	    r17		; shifted 4 bits left

			; Pack LEVEL into two bytes for output to the DAC in the sample loop
			sts		LEVEL_H, r17
			sts		LEVEL_L, r16