
;-------------------------------------------------------------------------------------------------------------------
;                    _     _  _                                _       
;                   | |   | |(_)                              | |      
;  ____  _____ _____| |__ | | _ ____     _____ ____   ___   __| |_____ 
; |    \| ___ | ___ |  _ \| || |  _ \   (____ |  _ \ / _ \ / _  | ___ |
; | | | | ____| ____| |_) ) || | |_| |  / ___ | | | | |_| ( (_| | ____|
; |_|_|_|_____)_____)____/ \_)_|  __/   \_____|_| |_|\___/ \____|_____)
;                              |_|                                     
; 
;-------------------------------------------------------------------------------------------------------------------
;
;Change log
;
;V1.03 2014.03.13 - Changed MIDI Set knobs to the bottom row. Width = Channel 1 .. Depth - Channel 4. 
;V1.02 2014.02.27 - Add Filter envelope modulation as MIDI CC parameter, LFO now continues during decay/release
;V1.01 2014.02.25 - Implement MIDI Channel set, MIDI CC parameter control
;V1.00 2014.01.14 - Initial release
;
;-------------------------------------------------------------------------------------------------------------------
;
;	MeeBlip Contributors
;
;	Jarek Ziembicki	- Created the original AVRsynth, upon which this project is based.
; 	Laurie Biddulph	- Worked with Jarek to translate his comments into English, ported to Atmega16
;	Daniel Kruszyna	- Extended AVRsynth (several of his ideas are incorporated in MeeBlip)
;  	Julian Schmidt	- Original Meeblip digital filter algorithm
; 	Axel Werner		- Code optimization, bug fixes and new bandlimited waveforms 
;	James Grahame 	- Meeblip Anode hardware and firmware development
;
;-------------------------------------------------------------------------------------------------------------------
;
;	Port Mapping 
;
;	PA0..7		8 potentiometers
;	PB5-PB7		ISP programming header / SPI communication
;	PC0-PC7		Switch inputs
;	PD0		    RxD (MIDI IN)
;	PD1		    Power ON/MIDI LED
;	PD3		    DAC CS (chip select)
;	PD4			LDAC (DAC load signal)
;	PD7			RAW Filter Cutoff control voltage (PWM generated)
;	
;
;	Timers	
;
;	Timer0		Sample timer: (Clock /8) / 50 --> 40000 Hz
;	Timer1		Time counter: CK/256		  --> TCNT1 
;	Timer2		PWM generated control voltage for VCF filter cutoff 
;
;-------------------------------------------------------------------------------------------------------------------

                    .NOLIST
                    .LIST
                    .LISTMAC

                    .SET cpu_frequency = 16000000
                    .SET baud_rate     = 31250
		            .SET KBDSCAN       = 6250	

;-------------------------------------------------------------------------------------------------------------------
;			V A R I A B L E S   &  D E F I N I T I O N S
;-------------------------------------------------------------------------------------------------------------------
;

.INCLUDE "variable_definitions.inc"

;-------------------------------------------------------------------------------------------------------------------
;			L O O K U P    T A B L E S
;-------------------------------------------------------------------------------------------------------------------
;
;			MIDI CC table
;			Interrupt vectors
;			Note table
;			VCF	curves
;			Time to Rate conversion for envelope timing
;			VCA curve
;

.INCLUDE "lookup_tables.inc"			

;-------------------------------------------------------------------------------------------------------------------
;		S A M P L E     G E N E R A T I O N     L O O P 
;-------------------------------------------------------------------------------------------------------------------
; Timer 1 compare interrupt (sampling)
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

.INCLUDE "sample_generation.inc"

;-------------------------------------------------------------------------------------------------------------------
;		M I D I 
;-------------------------------------------------------------------------------------------------------------------
;
; UART receiver (MIDI IN)
;

.INCLUDE "midi_in.inc"

;-------------------------------------------------------------------------------------------------------------------
;		S U B R O U T I N E S
;-------------------------------------------------------------------------------------------------------------------
;
;		ADC_START			- Get knob value
;		ADC_END				- Finish getting knob value
;		ASr16				- 16 bit arithmetic shift right
;		SHr32				- 32 bit logical shift right
;		SHL32				- 32 bit logical shift left
;		MUL32X16			- 32 bit x 16 bit multiplication (unsigned)
;		LOAD_32BIT			- Load 32 bit phase value from ROM
;		LOAD_DELTA			- Load phase delta from ROM
;		NOTERECALC			- Note number recalculation
;		TAB_BYTE			- Read byte from table
;		TAB_WORD			- Read word from table
;		ADCTORATE			- Time to Rate conversion
;		NONLINPOT			- Nonlinear potentiometer conversion (Oscillator detune)
;		EEPROM_WRITE		- Write byte to non-volatile eeprom memory
;		EEPROM_READ			- Read byte from non-volatile eeprom memory
;		SET_MIDI_CHANNEL	- Set new MIDI channel, reset LED flash
;		CLEAR_KNOB_STATUS	- Set knob status to 'unmoved' and save current knob positions
;		POT_SCAN			- Scan a knob and update its value if it has been moved
;		

.INCLUDE "subroutines.inc"

;-------------------------------------------------------------------------------------------------------------------
;			M A I N   P R O G R A M
;-------------------------------------------------------------------------------------------------------------------
;			 
;-------------------------------------------------------------------------------------------------------------------
; Main Program Loop
;
; This is where everything but sound generation happens. This loop is interrupted 40,000 times per second by the
; sample interrupt routine. When it's actually allowed to get down to work, it scans the panel switches every 32 ms,
; scans the knobs a lot more than that,  calculates envelopes, processes the LFO and parses MIDI input. 
; 
;-------------------------------------------------------------------------------------------------------------------
;


.INCLUDE "initialize.inc"				; Initialize variables, constants and start timer interrupts 

MAINLOOP:

; ------------------------------------------------------------------------------------------------------------------------
; Read switch values
; ------------------------------------------------------------------------------------------------------------------------
;

.INCLUDE "scan_switches.inc"

; ------------------------------------------------------------------------------------------------------------------------
; Read potentiometer values
; ------------------------------------------------------------------------------------------------------------------------
;

.INCLUDE "scan_knobs.inc"


;-------------------------------------------------------------------------------------------------------------------
; MIDI velocity 
;-------------------------------------------------------------------------------------------------------------------
; 
		
MIDI_VELOCITY:

			; Velocity control of filter
			lds 	R16, MIDIVELOCITY		; Value is 0..127
			lsl		r16
			lds		r17, VCFENVMOD
			mul		r16, r17
			sts		VELOCITY_ENVMOD, r1	
		
;-------------------------------------------------------------------------------------------------------------------


;-------------------------------------------------------------------------------------------------------------------
;			Calculate Time Delta
;-------------------------------------------------------------------------------------------------------------------
;
		    in	    r22, TCNT1L		    ;\
		    in	    r23, TCNT1H		    ;/ r23:r22 = TCNT1 = t
		    mov	    r18, r22		    ;\
    		mov	    r19, r23		    ;/ r19:r18 = t
		    lds	    r16, TPREV_L	    ;\
		    lds	    r17, TPREV_H	    ;/ r17:r16 = t0
		    sub	    r22, r16		    ;\ r23:r22 = t - t0 = dt
		    sbc	    r23, r17		    ;/ (1 bit = 32 µs)
		    sts	    TPREV_L, r18	    ;\
		    sts	    TPREV_H, r19	    ;/ t0 = t
    		sts	    DELTAT_L, r22		;\
		    sts	    DELTAT_H, r23		;/ r23:r22 = dT


;-------------------------------------------------------------------------------------------------------------------
;			LFO  
;-------------------------------------------------------------------------------------------------------------------
;
;			LFO1 : 	modulates oscillator pitch or filter cutoff
;			LFO2 :  sweeps PWM waveform duty cycle

.INCLUDE "lfo.inc"

;-------------------------------------------------------------------------------------------------------------------
;			Envelope Generation  
;-------------------------------------------------------------------------------------------------------------------
;
;			Envelope is routed to amplitude and filter cutoff

.INCLUDE "envelope.inc"

;-------------------------------------------------------------------------------------------------------------------
;			Note Handler  
;-------------------------------------------------------------------------------------------------------------------
;            
;			Processes note gating, portamento, note pitch

.INCLUDE "note_handler.inc"


;-------------------------------------------------------------------------------------------------------------------
;			Filter Modulation  
;-------------------------------------------------------------------------------------------------------------------
;            
;			Processes filter LFO modulation and enveloping

.INCLUDE "filter_modulation.inc"


;-------------------------------------------------------------------------------------------------------------------
;			DCA Output  
;-------------------------------------------------------------------------------------------------------------------
;            
;			Look up DCA value and output the DCA level amount to the second channel of the DAC chip. 

.INCLUDE "dca_output.inc"	

		

;-----------------------------------
;pseudo-random shift register:
;-----------------------------------
	        ;BIT = SHIFTREG.23 xor SHIFTREG.18
	        ;SHIFTREG = (SHIFTREG << 1) + BIT
		    lds	    r16, SHIFTREG_0
		    lds	    r17, SHIFTREG_1
		    lds	    r18, SHIFTREG_2
    		bst	    r18, 7			    ;\
		    bld	    r19, 0			    ;/ r19.0 = SHIFTREG.23
		    bst	    r18, 2			    ;\
		    bld	    r20, 0			    ;/ r20.0 = SHIFTREG.18
		    eor	    r19, r20			    ;r19.0 = BIT
		    lsr	    r19			        ; Cy = BIT
		    rol	    r16			        ;\
		    rol	    r17			        ; > r18:r17:r16 =
		    rol	    r18			        ;/  = (SHIFTREG << 1) + BIT
		    sts	    SHIFTREG_0, r16
		    sts	    SHIFTREG_1, r17
		    sts	    SHIFTREG_2, r18

;------------------------
;back to the main loop:
;------------------------
		    rjmp	MAINLOOP

;-------------------------------------------------------------------------------------------------------------------
;
;			Wavetable data  
;
;-------------------------------------------------------------------------------------------------------------------
;            
;			Bandlimited sawtooth wavetables (each table is 256 bytes long, unsigned integer)

.INCLUDE "wavetables.inc"

;-------------------------------------------------------------------------------------------------------------------

            .EXIT

;-------------------------------------------------------------------------------------------------------------------
