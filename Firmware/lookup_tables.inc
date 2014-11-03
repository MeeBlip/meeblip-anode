;-------------------------------------------------------------------------------------------------------------------
; Meeblip anode		Version 1.00
;
;
;-------------------------------------------------------------------------------------------------------------------
; MIDI Control Change parameter table
;-------------------------------------------------------------------------------------------------------------------
;
; MIDI CC parameters with an offset from MIDICC. They are automatically
; stored for use, just use the variable name to access their value. 
 

MIDICC:         		.byte $80 		; 128 bytes reserved in RAM for table
  .equ MIDIMODWHEEL		= MIDICC + $01

    ; Knobs - potentiometer 0 through 7
  .equ PANEL_LFOLEVEL 	= MIDICC + $30 
  .equ LFOFREQ			= MIDICC + $31
  .equ OSC_DETUNE 		= MIDICC + $32
  .equ PULSE_KNOB 		= MIDICC + $33
  .equ CUTOFF 			= MIDICC + $34
  .equ KNOB_AMP_DECAY 	= MIDICC + $35
  .equ KNOB_AMP_ATTACK	= MIDICC + $36
  .equ PORTAMENTO 		= MIDICC + $37 ; Not on front panel, but available through MIDI CC 
  .equ VCFENVMOD		= MIDICC + $38 ; Not on front panel, but available through MIDI CC

  ; Panel switches 0..7
  ; Switches 1
  .equ S_SUSTAIN		= MIDICC + $40
  .equ S_OSCB_OCT		= MIDICC + $41
  .equ S_PWM_SWEEP		= MIDICC + $42
  .equ S_LFO_DEST		= MIDICC + $43
  .equ S_LFO_RANDOM		= MIDICC + $44 
  .equ S_LFO_RETRIGGER	= MIDICC + $45 
  .equ S_OSCB_WAVE		= MIDICC + $46
  .equ S_UNASSIGNED_2	= MIDICC + $47


;-------------------------------------------------------------------------------------------------------------------



;stack: 0x0A3..0x25F
            .ESEG

;-------------------------------------------------------------------------------------------------------------------
;			V E C T O R   T A B L E
;-------------------------------------------------------------------------------------------------------------------
            .CSEG

		    jmp	RESET		            ; RESET

		    jmp	IRQ_NONE	            ; INT0
		    jmp	IRQ_NONE	            ; INT1
		    jmp	IRQ_NONE	            ; INT2

		    jmp	IRQ_NONE	            ; TIMEr2 COMP
		    jmp	IRQ_NONE	            ; TIMEr2 OVF

		    jmp	IRQ_NONE	            ; TIMEr1 CAPT
		    jmp	IRQ_NONE	            ; TIMEr1 COMPA
		    jmp	IRQ_NONE	            ; TIMEr1 COMPB
    		jmp	IRQ_NONE	            ; TIMEr1 OVF

		    jmp	TIM0_CMP	            ; TIMEr0 COMPA
		    jmp	IRQ_NONE	            ; TIMEr0 OVF

		    jmp	IRQ_NONE	            ; SPI,STC

		    jmp	UART_RXC	            ; UART, RX COMPLETE
		    jmp	IRQ_NONE	            ; UART,UDRE
		    jmp	IRQ_NONE	            ; UART, TX COMPLETE

		    jmp	IRQ_NONE	            ; ADC CONVERSION COMPLETE

		    jmp	IRQ_NONE	            ; EEPROM READY

		    jmp	IRQ_NONE	            ; ANALOG COMPARATOR

            jmp IRQ_NONE                ; 2-Wire Serial Interface

            jmp IRQ_NONE                ; STORE PROGRAM MEMORY READY

IRQ_NONE:
            reti
;-------------------------------------------------------------------------------------------------------------------
;			R O M   T A B L E S
;-------------------------------------------------------------------------------------------------------------------
;
; Note Phase Deltas at 40 kHz sample rate
;
;  				NOTE PHASE DELTA = 2 ^ 24 * Freq / SamplingFreq
;   	So... 	Note zero calc: 2 ^ 24 * 8.175799 / 40000 = 3429.17864 (stored as 00 0D 65.2E)
;-------------------------------------------------------------------------------------------------------------------

    
DELTA_C:
            .DW	0x652E		            ;\
		    .DW	0x000D		            ;/ note  0 ( 8.175799 Hz) 

DELTA_CIS:
            .DW	0x3117		            ;\
		    .DW	0x000E		            ;/ note  1 ( 8.661957 Hz) 

DELTA_D:
            .DW	0x091F		            ;\
		    .DW	0x000F		            ;/ note  2 ( 9.177024 Hz) 

DELTA_DIS:
            .DW	0xEE01		            ;\
		    .DW	0x000F		            ;/ note  3 ( 9.722718 Hz) 

DELTA_E:
            .DW	0xE07F		            ;\
		    .DW	0x0010		            ;/ note  4 (10.300861 Hz) 

DELTA_F:
            .DW	0xE167		            ;\
		    .DW	0x0011		            ;/ note  5 (10.913382 Hz) 

DELTA_FIS:
            .DW	0xF197		            ;\
		    .DW	0x0012		            ;/ note  6 (11.562326 Hz) 

DELTA_G:
            .DW	0x11F6		            ;\
		    .DW	0x0014		            ;/ note  7 (12.249857 Hz) 

DELTA_GIS:
            .DW	0x437B		            ;\
		    .DW	0x0015		            ;/ note  8 (12.978272 Hz) 

DELTA_A:
            .DW	0x872B		            ;\
		    .DW	0x0016		            ;/ note  9 (13.750000 Hz) 

DELTA_AIS:
            .DW	0xDE1A		            ;\
		    .DW	0x0017		            ;/ note 10 (14.567618 Hz) 

DELTA_H:
            .DW	0x496D		            ;\
		    .DW	0x0019		            ;/ note 11 (15.433853 Hz) 

DELTA_C1:
            .DW	0xCA5B		            ;\
		    .DW	0x001A		            ;/ note 12 (16.351598 Hz) 


;-----------------------------------------------------------------------------
;
; Lookup Tables
;
; VCF filter cutoff - 128 bytes
; Time to Rate table for calculating amplitude envelopes - 64 bytes
; VCA non-linear level conversion - 256 bytes
;
;-----------------------------------------------------------------------------
; VCF Filter Cutoff
;
; value = (16th root of 2)**(index+24)
;

; Digital VCF table (starts at zero)

TAB_VCF:
	.db	  1,   1,   1,   1,   1,   1,   1,   1		;   0
	.db	  1,   1,   1,   1,   1,   1,   1,   2		;   8
	.db	  2,   2,   2,   2,   2,   2,   2,   2		;  16
	.db	  2,   3,   3,   3,   3,   3,   3,   3		;  24
	.db	  4,   4,   4,   4,   4,   5,   5,   5		;  32
	.db	  5,   6,   6,   6,   7,   7,   7,   7		;  40
	.db	  8,   8,   9,   9,   9,  10,  10,  11		;  48
	.db	 11,  12,  12,  13,  14,  14,  15,  16		;  56
	.db	 16,  17,  18,  19,  19,  20,  21,  22		;  64
	.db	 23,  24,  25,  26,  28,  29,  30,  31		;  72
	.db	 33,  34,  36,  38,  39,  41,  43,  45		;  80
	.db	 47,  49,  51,  53,  56,  58,  61,  63		;  88
	.db	 66,  69,  72,  76,  79,  82,  86,  90		;  96
	.db	 94,  98, 103, 107, 112, 117, 122, 127		; 104
	.db	133, 139, 145, 152, 158, 165, 173, 181		; 112
	.db	189, 197, 206, 215, 224, 234, 245, 255		; 120

; Analog VCF table (offset to start at 25)
TAB_VCF2:
	.db	 25, 25, 25, 25, 25, 25, 25, 25
	.db	 25, 25, 25, 25, 25, 25, 25, 26
	.db	 26, 26, 26, 26, 26, 26, 26, 26
	.db	 26, 27, 27, 27, 27, 27, 27, 27
	.db	 28, 28, 28, 28, 28, 29, 29, 29
	.db	 29, 29, 29, 29, 30, 30, 30, 30
	.db	 31, 31, 32, 32, 32, 33, 33, 34
	.db	 34, 35, 35, 36, 37, 37, 38, 38
	.db	 38, 39, 40, 41, 41, 42, 43, 44
	.db	 45, 46, 47, 47, 49, 50, 51, 52
	.db	 54, 55, 56, 58, 59, 61, 63, 65
	.db	 66, 68, 70, 72, 75, 76, 79, 81
	.db	 84, 86, 89, 93, 95, 98, 102, 105
	.db	 109, 112, 117, 121, 125, 130, 134, 139
	.db	 144, 149, 155, 161, 167, 173, 180, 187
	.db	 194, 202, 210, 218, 226, 235, 245, 255


;-----------------------------------------------------------------------------
; 
; Time to Rate conversion table for envelope timing.
; 
; lfo:
;  update values for 32us update rate
;  LFO_INTEGR overflows all 256*32us = 8.192 ms
;
; formula  Tof = 256*32us*2^16/N
;          LFOfreq = 1/Tof

; Rate value =	Rmin * Q^i with Q = (Rmax/Rmin)^(1/31) = 1,286111766

TIMETORATE:
	.DW		25479	; 10.54 mS	fast lfo, attack/rise time
	.DW		19811	; 13.55 mS
	.DW		15404	; 17.43 mS
	.DW		11977	; 22.41 mS
	.DW		9313	; 28.83 mS
	.DW		7241	; 37.07 mS
	.DW		5630	; 47.68 mS
	.DW	 	4378	; 61.32 mS
	.DW	 	3404	; 78.87 mS
	.DW	 	2647	; 101.4 mS
	.DW	 	2058	; 130.5 mS
	.DW	 	1600	; 167.8 mS
	.DW	 	1244	; 215.8 mS
	.DW	 	968	; 277.5 mS
	.DW	 	752 ; 356.9 mS
	.DW	 	585 ; 459.0 mS
	.DW	  	455	; 590.4 mS
	.DW	  	354	; 759.3 mS
	.DW	  	275	; 976.5 mS
	.DW	  	214	; 1.256 S
	.DW	  	166	; 1.615 S
	.DW	  	129	; 2.077 S
	.DW	  	101	; 2.672 S
	.DW	  	78	; 3.436 S
	.DW	  	61	; 4.419 S
	.DW	   	47	; 5.684 S
	.DW	   	37	; 7.310 S
	.DW	  	29	; 9.401 S
	.DW	   	22	; 12.09 S
	.DW	   	18	; 15.55 S
	.DW	   	14	; 20.00 S
	.DW	   	10	; 28.26 S	slow lfo, attack/rise time


;-----------------------------------------------------------------------------
;
; VCA non-linear level conversion 
;
; Amplitude level lookup table. Envelopes levels are calculated as linear 
; and then converted to approximate an exponential saturation curve.
;
; polynomial y = a	+ bx + cx2 + dx3
; with coefficients…
;    a  0
;    b  0.210841569
;    c  0.000177823
;    d  1.14E-05

TAB_VCA:
	.db	  0,   0,   0,   1,   1,   1,   1,   1		;   0
	.db	  2,   2,   2,   2,   3,   3,   3,   3		;   8
	.db	  3,   4,   4,   4,   4,   5,   5,   5		;  16
	.db	  5,   6,   6,   6,   6,   7,   7,   7		;  24
	.db	  7,   8,   8,   8,   8,   9,   9,   9		;  32
	.db	  9,  10,  10,  10,  11,  11,  11,  11		;  40
	.db	 12,  12,  12,  13,  13,  13,  14,  14		;  48
	.db	 14,  15,  15,  15,  16,  16,  16,  17		;  56
	.db	 17,  18,  18,  18,  19,  19,  20,  20		;  64
	.db	 20,  21,  21,  22,  22,  23,  23,  23		;  72
	.db	 24,  24,  25,  25,  26,  26,  27,  27		;  80
	.db	 28,  28,  29,  29,  30,  30,  31,  31		;  88
	.db	 32,  33,  33,  34,  34,  35,  35,  36		;  96
	.db	 37,  37,  38,  39,  39,  40,  41,  41		; 104
	.db	 42,  43,  43,  44,  45,  45,  46,  47		; 112
	.db	 48,  48,  49,  50,  51,  51,  52,  53		; 120

	.db	 54,  55,  56,  56,  57,  58,  59,  60		; 128
	.db	 61,  62,  63,  63,  64,  65,  66,  67		; 136
	.db	 68,  69,  70,  71,  72,  73,  74,  75		; 144
	.db	 76,  77,  78,  80,  81,  82,  83,  84		; 152
	.db	 85,  86,  87,  89,  90,  91,  92,  93		; 160
	.db	 95,  96,  97,  98, 100, 101, 102, 104		; 168
	.db	105, 106, 108, 109, 110, 112, 113, 115		; 176
	.db	116, 118, 119, 120, 122, 123, 125, 126		; 184
	.db	128, 130, 131, 133, 134, 136, 138, 139		; 192
	.db	141, 142, 144, 146, 148, 149, 151, 153		; 200
	.db	154, 156, 158, 160, 162, 164, 165, 167		; 208
	.db	169, 171, 173, 175, 177, 179, 181, 183		; 216
	.db	185, 187, 189, 191, 193, 195, 197, 199		; 224
	.db	201, 203, 206, 208, 210, 212, 214, 217		; 232
	.db	219, 221, 224, 226, 228, 231, 233, 235		; 240
	.db	238, 240, 243, 245, 247, 250, 252, 255		; 248