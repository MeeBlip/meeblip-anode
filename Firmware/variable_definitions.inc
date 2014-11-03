;-------------------------------------------------------------------------------------------------------------------
; Meeblip anode		Version 1.00
;
;
;-------------------------------------------------------------------------------------------------------------------
; Variables and definitions
;-------------------------------------------------------------------------------------------------------------------
;

;registers:

;current phase of DCO A:
.DEF PHASEA_0	    = 	r2
.DEF PHASEA_1	    = 	r3
.DEF PHASEA_2	    = 	r4

;current phase of DCO B:
.DEF PHASEB_0	    = 	r5
.DEF PHASEB_1	    = 	r6
.DEF PHASEB_2	    = 	r7

.DEF ZERO           =   r8

;DCF:

.def a_L 			= r9
.def a_H 			= r10
.def temp_SREG		= r11
.def z_L 			= r18
.def z_H 			= r19
.def temp	 		= r30
.def temp2			= r31

.DEF OLD_LDAC  = 	r14 ; previous sample
.DEF OLD_HDAC  = 	r15 ; previous sample

.def LDAC			= r16
.def HDAC			= r17

;RAM (0060h...025Fh):

                    .DSEG
;MIDI:
MIDIPHASE:          .BYTE 1
MIDICHANNEL:        .BYTE 1
MIDIDATA0:	        .BYTE 1
MIDIVELOCITY:	    .BYTE 1
MIDINOTE:	        .BYTE 1
LFONOTE:			.BYTE 1
MIDINOTEPREV:	    .BYTE 1		        ; buffer for MIDI note
MIDIPBEND_L:        .BYTE 1		        ;\
MIDIPBEND_H:        .BYTE 1		        ;/ -32768..+32766

;current sound parameters:
LFOLEVEL:	        .BYTE 1	            ; 0..255
KNOB_SHIFT:			.BYTE 1				; 0= unchanged 255= changed state  
POWER_UP:			.BYTE 1				; 255 = Synth just turned on, 0 = normal operation
KNOB0_STATUS:		.BYTE 1				; Each byte corresponds to a panel knob.
KNOB1_STATUS:		.BYTE 1				; 0 = pot not updated since Knob Shift switch change
KNOB2_STATUS:		.BYTE 1				; 1 = pot has been updated. 
KNOB3_STATUS:		.BYTE 1
KNOB4_STATUS:		.BYTE 1
KNOB5_STATUS:		.BYTE 1
KNOB6_STATUS:		.BYTE 1
KNOB7_STATUS:		.BYTE 1

SWITCH1:	        .BYTE 1	            
SWITCH2:	        .BYTE 1	            
OLD_SWITCH1:		.BYTE 1				; Previous switch values (used to flag switch changes)
OLD_SWITCH2:		.BYTE 1

										
										; Switch value currently used (from front panel, MIDI or last loaded patch)
PATCH_SWITCH1:		.BYTE 1				
  .equ SW_SUSTAIN		= 0
  .equ SW_OSCB_OCT		= 1
  .equ SW_PWM_SWEEP		= 2
  .equ SW_LFO_DEST		= 3
  .equ SW_LFO_RANDOM	= 4
  .equ SW_LFO_RETRIGGER	= 5
  .equ SW_OSCB_WAVE		= 6
  .equ SW_UNASSIGNED_2	= 7




PATCH_SWITCH2:		.BYTE 1
  .equ SW_MIDI			= 0

SETMIDICHANNEL:	    .BYTE 1             ; selected MIDI channel: 0 for OMNI or 1..15
DETUNEB_FRAC:	    .BYTE 1	            ;\
DETUNEB_INTG:	    .BYTE 1	            ;/ -128,000..+127,996
ATTACKTIME:	        .BYTE 1	            ; 0..255
DECAYTIME:			.BYTE 1				; 0..255
SUSTAINLEVEL:		.BYTE 1				; 0/255
RELEASETIME:        .BYTE 1	            ; 0..255
ATTACKTIME2:	    .BYTE 1				; 0..255
DECAYTIME2:			.BYTE 1				; 0..255
SUSTAINLEVEL2:		.BYTE 1				; 0/255
RELEASETIME2:        .BYTE 1	        ; 0..255
NOTE_L:		        .BYTE 1
NOTE_H:		        .BYTE 1
NOTE_INTG:	        .BYTE 1
PORTACNT:	        .BYTE 1		        ; 2 / 1 / 0
LPF_I:		        .BYTE 1
LEVEL:		        .BYTE 1		        ; 0..255
PITCH:		        .BYTE 1		        ; 0..96
ADC_CHAN:	        .BYTE 1		        ; 0..7
PREV_ADC_CHAN:		.BYTE 1				; 0..7
ADC_0:		        .BYTE 1				; Panel knob values.
ADC_1:		        .BYTE 1
ADC_2:		        .BYTE 1
ADC_3:		        .BYTE 1
ADC_4:		        .BYTE 1
ADC_5:		        .BYTE 1
ADC_6:		        .BYTE 1
ADC_7:		        .BYTE 1
OLD_ADC_0:			.BYTE 1				; Previous panel knob value
OLD_ADC_1:			.BYTE 1
OLD_ADC_2:			.BYTE 1
OLD_ADC_3:			.BYTE 1
OLD_ADC_4:			.BYTE 1
OLD_ADC_5:			.BYTE 1
OLD_ADC_6:			.BYTE 1
OLD_ADC_7:			.BYTE 1
GATE:		        .BYTE 1		        ; 0 / 1
GATEEDGE:	        .BYTE 1		        ; 0 / 1
TPREV_KBD_L:	    .BYTE 1
TPREV_KBD_H:	    .BYTE 1
TPREV_L:	        .BYTE 1
TPREV_H:	        .BYTE 1
DELTAT_L:	        .BYTE 1		        ;\ Time from former course
DELTAT_H:	        .BYTE 1		        ;/ of the main loop (1 bit = 32 µs)
ENVPHASE:	        .BYTE 1		        ; 0=stop 1=attack 2=decay 3=sustain 4=release
ENV_FRAC_L:	        .BYTE 1
ENV_FRAC_H:	        .BYTE 1
ENV_INTEGR:	        .BYTE 1
ENVPHASE2:	        .BYTE 1		        ; 0=stop 1=attack 2=decay 3=sustain 4=release
ENV_FRAC_L2:	    .BYTE 1
ENV_FRAC_H2:	    .BYTE 1
ENV_INTEGR2:	    .BYTE 1
VELOCITY_ENVMOD:	.BYTE 1
OLD_CUTOFF:			.BYTE 1

LFOPHASE:	        .BYTE 1		        ; 0=up 1=down
LFO_FRAC_L:	        .BYTE 1		        ;\
LFO_FRAC_H:	        .BYTE 1		        ; > -128,000..+127,999
LFO_INTEGR:	        .BYTE 1		        ;/
LFOVALUE:	        .BYTE 1		        ; -128..+127
LFO2PHASE:	        .BYTE 1		        ; 0=up 1=down
LFO2_FRAC_L:	    .BYTE 1		        ;\
LFO2_FRAC_H:	    .BYTE 1		        ; > -128,000..+127,999
LFO2_INTEGR:	    .BYTE 1		        ;/
LFO2VALUE:	        .BYTE 1		        ; -128..+127
OLDWAVEA:	        .BYTE 1
OLDWAVEB:	        .BYTE 1
SHIFTREG_0:	        .BYTE 1		        ;\
SHIFTREG_1:	        .BYTE 1		        ; > shift register for
SHIFTREG_2:	        .BYTE 1		        ;/  pseudo-random generator
LFOBOTTOM_0:        .BYTE 1		        ;\
LFOBOTTOM_1:        .BYTE 1		        ; > bottom level of LFO
LFOBOTTOM_2:        .BYTE 1		        ;/
LFOTOP_0:	        .BYTE 1		        ;\
LFOTOP_1:	        .BYTE 1		        ; > top level of LFO
LFOTOP_2:	        .BYTE 1		        ;/
LFO2BOTTOM_0:       .BYTE 1		        ;\
LFO2BOTTOM_1:       .BYTE 1		        ; > bottom level of LFO2
LFO2BOTTOM_2:       .BYTE 1		        ;/
LFO2TOP_0:	        .BYTE 1		        ;\
LFO2TOP_1:	        .BYTE 1		        ; > top level of LFO2
LFO2TOP_2:	        .BYTE 1		        ;/
DCOA_LEVEL:			.BYTE 1	
DCOB_LEVEL:			.BYTE 1	
KNOB_DEADZONE:		.BYTE 1

; increase phase for DCO A
DELTAA_0: .byte 1
DELTAA_1: .byte 1
DELTAA_2: .byte 1

; increase phase for DCO B
DELTAB_0: .byte 1
DELTAB_1: .byte 1
DELTAB_2: .byte 1

; Wavetable select
WAVETABLE_A:		.byte 1		; Bandlimited wavetable 0..11
WAVETABLE_B:		.byte 1		; Bandlimited wavetable 0..11
WAVETABLE:			.byte 1		; User wavetable 0.32


; oscillator pulse width
PULSE_WIDTH:		.byte 1
PULSE_KNOB_LIMITED:	.byte 1

; fm
WAVEB:	  .byte 1
FMDEPTH:  .byte 1

; eeprom 
WRITE_MODE:			.byte 1
WRITE_OFFSET:		.byte 1		; byte 0..15 of the patch
WRITE_PATCH_OFFSET:	.byte 1		; start of patch in eeprom

; filter
SCALED_RESONANCE: .byte 1
b_L:		.byte 1
b_H:		.byte 1
VCF_STATUS: .byte 1				; 0 indicates VCF off, 1 = on

; DCA LEVEL
LEVEL_L:	.byte 1
LEVEL_H:	.byte 1				; Envelope LEVEL packed in 2 bytes ready for the DCA. 

; Filter Cutoff Level
LPF_L:		.byte 1
LPF_H:		.byte 1				; Filter cutoff packed in 2 bytes for DAC

; Patch save/load and MIDI channel set
LED_TIMER:		.byte 1				; number of blinks before reset
LED_TIMER2:		.byte 1
BUTTON_STATUS:	.byte 1				; MIDI=1
FIRMWARE:		.byte 1				; 0 = standard firmware, 1 = alternate