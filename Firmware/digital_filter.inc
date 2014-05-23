;-------------------------------------------------------------------------------------------------------------------
; Meeblip anode		Version 1.10
;
;
;-------------------------------------------------------------------------------------------------------------------
; Digitally Controlled Filter
;-------------------------------------------------------------------------------------------------------------------
; 
; A 2-pole resonant low pass filter:
;
; a += f * ((in - a) + q * (a - b));
; b += f * (a - b); 
;
; Input 16-Bit signed HDAC:LDAC (r17:r16), already scaled to minimize clipping (reduced to 25% of full code).
;-------------------------------------------------------------------------------------------------------------------
;
	;calc (in - a) ; both signed
	sub	LDAC, a_L
	sbc	HDAC, a_H
					;check for overflow / do hard clipping
	brvc	OVERFLOW_1		;if overflow bit is clear jump to OVERFLOW_1

					;sub overflow happened -> set to min
					;0b1000.0000 0b0000.0001 -> min
					;0b0111.1111 0b1111.1111 -> max

	ldi	LDAC, 0b00000001
	ldi	HDAC, 0b10000000

OVERFLOW_1:				;when overflow is clear

					;(in-a) is now in HDAC:LDAC as signed
					;now calc q*(a-b)

					; Scale resonance based on filter cutoff
	;lds	r22, RESONANCE
	ldi		r22, 0
	lds	r20, LPF_I		;load 'F' value
	ldi	r21, 0xff

	sub	r21, r20		; 1-F
	lsr	r21
	ldi	r18, 32
	add	r21, r18

	sub	r22, r21		; Q-(1-f)
	brcc	OVERFLOW_2		; if no overflow occured
	ldi	r22, 0x00		;0x00 because of unsigned

OVERFLOW_2:

	mov	r20, a_L		;\
	mov	r21, a_H		;/ load 'a' , signed

	lds	z_H, b_H		;\
	lds	z_L, b_L		;/ load 'b', signed

	sub	r20, z_L		;\
	sbc	r21, z_H		;/ (a-b) signed

	brvc	OVERFLOW_3		;if overflow is clear jump to OVERFLOW_3

					;0b1000.0000 0b0000.0001 -> min
					;0b0111.1111 0b1111.1111 -> max

	ldi	r20, 0b00000001
	ldi	r21, 0b10000000

OVERFLOW_3:

CALC_LOWPASS:
					; mul signed:unsigned -> (a-b) * Q
					; 16x8 into 16-bit
					; r19:r18 = r21:r20 (ah:al) * r22 (b)

	mulsu	r21, r22		; (signed)ah * b
	movw	r18, r0
	mul	r20, r22		; al * b
	add	r18, r1
	adc	r19, ZERO
	rol	r0			; r0.7 --> Cy
	brcc	NO_ROUND		; LSByte < $80, so don't round up
	inc	r18
NO_ROUND:
	clc
	lsl	r18
	rol	r19
	clc
	lsl	r18
	rol	r19
	movw	z_L,r18			;Q*(a-b) in z_H:z_L as signed

					;add both
					;both signed
					;((in-a)+q*(a-b))
					;=> HDAC:LDAC + z_H:z_L

 DCF_ADD:

	add	LDAC, z_L
	adc	HDAC, z_H

	brvc	OVERFLOW_4		;if overflow is clear
					;0b1000.0000 0b0000.0001 -> min
					;0b0111.1111 0b1111.1111 -> max

	ldi	LDAC, 0b11111111
	ldi	HDAC, 0b01111111

OVERFLOW_4:

					;Result is a signed value in HDAC:LDAC
					;calc * f
					;((in-a)+q*(a-b))*f

	lds	r20, LPF_I		;load lowpass 'F' value
					; mul signed unsigned HDAC*F
					; 16x8 into 16-bit
					; r19:r18 = HDAC:LDAC (ah:al) * r20 (b)

	mulsu	HDAC, r20		; (signed)ah * b
	movw	r18, r0
	mul	LDAC, r20		; al * b
	add	r18, r1			; signed result in r19:r18
	adc	r19, ZERO
	rol	r0			; r0.7 --> Cy
	brcc	NO_ROUND2		; LSByte < $80, so don't round up
	inc	r18
NO_ROUND2:
					;Add result to 'a'
					;a+=f*((in-a)+q*(a-b))

	add	a_L, r18
	adc	a_H, r19
	brvc	OVERFLOW_5		;if overflow is clear
					;0b1000.0000 0b0000.0001 -> min
					;0b0111.1111 0b1111.1111 -> max

	ldi	z_L, 0b11111111
	ldi	z_H, 0b01111111
	mov	a_L, z_L
	mov	a_H, z_H

OVERFLOW_5:
					;calculated a+=f*((in-a)+q*(a-b)) as signed value and saved in a_H:a_L
					;calc 'b'
					;b += f * (a*0.5 - b);

	mov	z_H, a_H		;\
	mov	z_L, a_L		;/ load 'a' as signed

	lds	temp, b_L		;\
	lds	temp2, b_H		;/ load b as signed

	sub	z_L, temp		;\
	sbc	z_H, temp2		;/ (a - b) signed

	brvc	OVERFLOW_6		;if overflow is clear
					;0b1000.0000 0b0000.0001 -> min
					;0b0111.1111 0b1111.1111 -> max

	ldi	z_L, 0b00000001
	ldi	z_H, 0b10000000

OVERFLOW_6:

	lds	r20, LPF_I		;load lowpass 'F' value

					; mul signed unsigned (a-b) * F
					; 16x8 into 16-bit
					; r19:r18 = z_H:z_L (ah:al) * r20 (b)
	mulsu	z_H, r20		; (signed)ah * b
	movw	r18, r0
	mul	z_L, r20		; al * b
	add	r18, r1			; signed result in r19:r18
	adc	r19, ZERO


	add	temp,  r18		;\ add result to 'b' , signed
	adc	temp2, r19		;/ b +=(a-b)*f

	brvc	OVERFLOW_7		;if overflow is clear

					;0b1000.0000 0b0000.0001 -> min
					;0b0111.1111 0b1111.1111 -> max

	ldi	temp,  0b11111111
	ldi	temp2, 0b01111111

OVERFLOW_7:

	sts	b_L, temp		;\
	sts	b_H, temp2		;/ save value of 'b'


	mov	LDAC, temp		;B now contains the filtered signal in HDAC:LDAC
	mov	HDAC, temp2		; output sample HDAC:LDAC = r17:r16