;-------------------------------------------------------------------------------------------------------------------
; Meeblip anode		Version 1.00
;
;-------------------------------------------------------------------------------------------------------------------
; Initialization
;

RESET:
            cli				            ; disable interrupts

;JTAG Disable - Set JTD in MCSCSR
            lds     r16, MCUCSR         ; Read MCUCSR
            sbr     r16, 1 << JTD       ; Set jtag disable flag
            out     MCUCSR, r16         ; Write MCUCSR
            out     MCUCSR, r16         ; and again as per datasheet

;initialize stack:
  			ldi 	r16, low(RAMEND)
			ldi 	r17, high(RAMEND)
		    out	    SPL, r16
		    out	    SPH, r17

;initialize variables:
		    clr	    ZERO
		    clr	    PHASEA_0
    		clr	    PHASEA_1
		    clr	    PHASEA_2
		    clr	    PHASEB_0
		    clr	    PHASEB_1
		    clr	    PHASEB_2
			clr 	a_L					; clear DCF registers
			clr 	a_H					;
			clr		z_L					;
			clr 	z_H					;
			clr 	temp				;
			clr 	temp2				;
			ldi		r16, 5
			sts 	KNOB_DEADZONE, r16	
		    ldi	    r16, 0
			sts		BUTTON_STATUS, r16	; no buttons pressed
			sts		FIRMWARE, r16		; Run standard firmware
			sts		PATCH_SWITCH1, r16	; Default patch is null until loaded from front panel or memory
			sts		SWITCH1, r16
			sts		SETMIDICHANNEL, r16 ; Default MIDI channel to zero (omni)
			sts		FMDEPTH, r16		; FM Depth = 0
		    sts	    GATE, r16		    ; GATE = 0
		    sts	    GATEEDGE, r16	    ; GATEEDGE = 0
		    sts	    LEVEL, r16		    ; LEVEL = 0
		    sts	    ENV_FRAC_L, r16	    ;\
		    sts	    ENV_FRAC_H, r16	    ; > ENV = 0
		    sts	    ENV_INTEGR, r16	    ;/
		    sts	    ADC_CHAN, r16	    ; ADC_CHAN = 0
		    sts	    NOTE_L, r16		    ;\
		    sts	    NOTE_H, r16		    ; >
		    sts	    NOTE_INTG, r16	    ;/
		    sts	    MIDIPBEND_L, r16    ;\
		    sts	    MIDIPBEND_H, r16    ;/ P.BEND = 0
		    sts	    MIDIMODWHEEL, r16   ; MOD.WHEEL = 0
			sts		KNOB_SHIFT, r16		; Initialize panel shift switch = 0 (unshifted)
			sts		VCF_STATUS, r16		; Flag VCF as off (0)
		    ldi	    r16, 2
		    sts	    PORTACNT, r16	    ; PORTACNT = 2
		    ldi	    r16, 255
			sts		POWER_UP, r16		; Set power_up flag to 255 to force first initialization of panel switches
			sts		WRITE_MODE, r16		; Patch write mode defaults to "off" 
		    sts	    LPF_I, r16		    ; no DCF
		    sts	    MIDINOTE, r16	    ; note# = 255
		    sts	    MIDINOTEPREV, r16   ; note# = 255
		    ldi	    r16, 0x5E		    ;\
		    ldi	    r17, 0xB4		    ; \
		    ldi	    r18, 0x76		    ;  \ initialising of
		    sts	    SHIFTREG_0, r16		;  / shift register
		    sts	    SHIFTREG_1, r17		; /
		    sts	    SHIFTREG_2, r18		;/
		    ldi	    r16, 0			    ;\
    		ldi	    r17, 0			    ; > Amin = 0
		    ldi	    r18, 0			    ;/
		    sts	    LFOBOTTOM_0, r16	;\
		    sts	    LFOBOTTOM_1, r17	; > store Amin for LFO
		    sts	    LFOBOTTOM_2, r18	;/
		    ldi	    r16, 255		    ;\
		    ldi	    r17, 255		    ; > Amax = 255,999
		    ldi	    r18, 255		    ;/
		    sts	    LFOTOP_0, r16		;\
		    sts	    LFOTOP_1, r17		; > store Amax for LFO
		    sts	    LFOTOP_2, r18		;/
			ldi		r18, 20
			sts	    LFO2BOTTOM_0, r16	;\
		    sts	    LFO2BOTTOM_1, r17	; > store Amin for LFO2
			sts	    LFO2BOTTOM_2, r18	;/
			ldi		r18, 100
			sts	    LFO2TOP_0, r16		;\
		    sts	    LFO2TOP_1, r17		; > store Amax for LFO2
		    sts	    LFO2TOP_2, r18		;/

			ldi		r16, 1 
			sts		KNOB0_STATUS, r16		; Flag all knobs so current panel value is loaded upon power up.  
			sts		KNOB1_STATUS, r16		; = 1
			sts		KNOB2_STATUS, r16		; = 1
			sts		KNOB3_STATUS, r16		; = 1
			sts		KNOB4_STATUS, r16		; = 1
			sts		KNOB5_STATUS, r16		; = 1
			sts		KNOB6_STATUS, r16		; = 1
					
			; default ENV MOD to 24 
			ldi		r16, 24
			sts		VELOCITY_ENVMOD, r16

			ldi		r16, 255
			sts		PATCH_SWITCH2, r16	; Default MIDI set switch to unpressed.

			ldi		r16, 0b10110000
			sts		LEVEL_H, r16
			ldi		r16, 0b00000000
			sts		LEVEL_L, r16		; Default envelope level to zero

;initialize sound parameters:
			ldi	    r16,0
		    sts	    LFOPHASE, r16		;
			sts	    LFO2PHASE, r16		;
		    sts	    ENVPHASE, r16		;
		    sts	    DETUNEB_FRAC, r16	;\
		    sts	    DETUNEB_INTG, r16	;/ detune = 0
		    sts	    LFOLEVEL, r16		;
			sts		LFOFREQ, r16
			sts		PORTAMENTO, r16
			ldi		r16, 100
			sts		VCFENVMOD, r16		; Default VCFENVMOD to 100 unless overridden by MIDI CC

;initialize port A:
		    ldi	    r16, 0x00    		;\
			out	    PORTA, r16		    ;/ PA = zzzzzzzz
		    ldi	    r16, 0x00    		;\
		    out	    DDRA, r16		    ;/ PA = iiiiiiii    all inputs (panel pots)

;initialize port B:
		    ldi	    r16, 0xFF    		;\
		    out	    PORTB, r16		    ;/ PB = pppppppp
		    ldi	    r16, 0x00    	    ;\
		    out	    DDRB, r16		    ;/ PB = iiiiiiii    all inputs

; SPI outputs on port B

;			ldi		r16,(1<<DDB5)|(1<<DDB7) ; MOSI and SCK as outputs
			ldi		r16,(1<<DDB5)|(1<<DDB7)|(1<<DDB4) ; MOSI, SS and SCK as outputs
			out		DDRB, r16

			; Enable SPI, Master, set clock rate fck/2
			ldi		r16,(1<<SPE)|(1<<MSTR)	; fck/4
			out		SPCR, r16

			ldi		r16, (1<<SPI2X)			; fck/2 (double SPI speed)
			out		SPSR, r16

;initialize port C:
		    ldi	    r16, 0x00    	    ;\
		    out	    DDRC, r16		    ;/ PB = iiiiiiii    all inputs, pull-ups enabled
		    ldi	    r16, 0xFF    		;\
		    out	    PORTC, r16		    ;/ PB = pppppppp


;initialize port D:
		    ldi	    r16, 0b11111110    		;\
		    out	    DDRD, r16		    ;/ PD = oooooooi    all outputs except PD0 (MIDI-IN) 
			;ldi	    r16, 0b11111110    		;\
			ldi		r16, 0b00000000
		    out	    PORTD, r16		    ;/ Disable pull-ups for Port D
; Turn Power/MIDI LED on at power up
			
			sbi	    PORTD, 1		    ; LED on

;initialize Timer0:

            ldi     r16, 49             ;\  ; Set sample rate to 40.00 kHz
            out     OCr0, r16           ;/ OCr0 = 49 gives 40000.00 Hz sample rate at 400 cycles per sample loop.
            ldi     r16, 0x0A           ;\ clear timer on compare,
            out     TCCr0, r16          ;/ set prescaler = CK/8

;initialize Timer1:
		    ldi	    r16, 0x04    		;\ prescaler = CK/256
		    out	    TCCr1B, r16		    ;/ (clock = 32µs)

;Timer2 PWM controlling cutoff
            ldi     r16, 25              ; Set filter cutoff to minimum value 
            out     OCr2, r16			; load output compare register 
			ldi		r16, 0b01100001		; Phase correct PWM, no prescaler
		    out	    TCCr2, r16			; start PWM

;initialize UART:
		    ldi	    r16, high((cpu_frequency / (baud_rate * 16)) - 1)
		    out	    UBRRH, r16
    		ldi	    r16, low((cpu_frequency / (baud_rate * 16)) - 1)
            out     UBRRL, r16

; enable receiver and receiver interrupt
    		ldi	    r16, (1<<RXCIE)|(1<<RXEN)   ;\
		    out	    UCR, r16		            ;/ RXCIE=1, RXEN=1

;initialize ADC:
		    ldi	    r16, 0x86    		;\
		    out	    ADCSRA, r16		    ;/ ADEN=1, clk = 125 kHz

;initialize interrupts:
		    ldi	    r16, 0x02    		;\
		    out	    TIMSK, r16		    ;/ OCIE0=1

    		sei				            ; Interrupt Enable

;start conversion of the first A/D channel:
		    lds	    r18, ADC_CHAN
		    rcall	ADC_START

; Load MIDI channel from eeprom

			ldi		r18, $03
			ldi		r17, $FF				; Set eeprom address to $03FF (last byte)
			rcall   EEPROM_READ				; load MIDI CHANNEL into r16
			cpi		r16, 5					
			brlo	LOAD_MIDI_CHANNEL		; If stored MIDI Channel is not Channel 1-4, set synth into MIDI OMNI mode.
			ldi		r16, 0			
LOAD_MIDI_CHANNEL:
			sts		SETMIDICHANNEL, r16

;initialize the keyboard scan time 
		    in		r16, TCNT1L		        ;\
		    in		r17, TCNT1H		        ;/ r17:r16 = TCNT1 = t
		    sts		TPREV_KBD_L, r16
		    sts		TPREV_KBD_H, r17

; Alternate firmware check
; Check if MIDI SET button is pressed on back panel at power up. If so, run alternate firmware.
			in		r16, PINC	; Read PORTC pin values (front panel switches)	
			sbrc	r16, 0				
			rjmp	STANDARD_FIRMWARE	; skip if MIDI Set button = 1 (not pressed)
			
			ldi		r16, 1
			sts		FIRMWARE, r16 		; Flag alternate FIRMWARE (set to 1)
			
			; blink LED double fast if alternate firmware has been selected. 
			ldi		r16, 3
			sts		BUTTON_STATUS, r16	; Save flash status, turn on LED flash timer 
			ldi		r16, 255
			sts		LED_TIMER, r16
			ldi		r16, 48
			sts		LED_TIMER2, r16
			
STANDARD_FIRMWARE:						; Do nothing, run standard firmware options.