

.def	chksumL		= r0
.def	chksumM		= r1
.def	chksumH		= r2

.def	WL		= r16
.def	WH		= r17

.def	valueL		= r20
.def	valueH		= r21

.def	lengthL		= r26
.def	lengthH		= r27


; =======================================================================
; == IP =================================================================
; =======================================================================

;
; Calc checksum. Reads packet at offset zL/zH for
; lengthH/lengthL bytes and calculates the checksum
; in valueH/valueL.
;
cksum: 
	clr     chksumL		;we do the arithmetic
	clr     chksumM		; using a 24
	clr     chksumH		;  bit area
	sbrs    lengthL,0	;odd length?
	rjmp	cksumC
	mov	YL,ZL
	mov	YH,ZH
	add	YL,lengthL
	adc	YH,lengthH
    	clr	WL
	st	y,WL		       	;clear byte after last
	adiw	lengthL,1
cksumC: 
	lsr	lengthH
    	ror	lengthL
cksuml: 
	ld      WH,z+		   ;get high byte of 16-bit word
	ld      WL,z+
	add     chksumL,WL	      ;add to accum
	brcc    noLcarry
	ldi     WL,1
	add     chksumM,WL
	brcc    noLcarry
	add     chksumH,WL
noLcarry:
	add     chksumM,WH	      ;add in the high byte
	brcc    noHcarry
	inc     chksumH
noHcarry:
	subi    lengthL,1
	sbci    lengthH,0
	clr     WL
	cpi     lengthL,0
	cpc     lengthH,WL
	breq    CkDone
	brpl    cksuml    
CkDone: 
	add     chksumL,chksumH	 ;add in the third byte of 24 bit area
	brcc    CkDone1
	inc     chksumM
CkDone1:
	mov     valueL,chksumL
	com	valueL
	mov     valueH,chksumM
	com	valueH
	ret
