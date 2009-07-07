.include "m162def.inc"

.equ	EGPIO_TP1		= 0x7			; Serial TX Output
.equ	EGPIO_SET		= 0x80		; flag to set
.equ	EGPIO_BASE		= 0xc000


.equ	STACK_TOP		= 0x200
.def	tmp			= r24

; =======================================================================
; == Set the initial vectors ============================================
; =======================================================================

jmp	reset						; Reset Vector
nop							; Int0
nop							; Int1
nop



; =======================================================================
; == Initialisation =====================================================
; =======================================================================

reset:
	
	; select 8MHz clock
	ldi	r16, 0x80
	sts	CLKPR, r16
	clr	r16
	sts	CLKPR, r16
 
	; set up stack pointer
	ldi	r16, (STACK_TOP & 0xff)
	out	SPL, r16	
	ldi	r16, (STACK_TOP >> 8)
	out	SPH, r16

	; set up external memory interface and interrupts
	; select 1 wait state for upper region at 0x8000-0xffff
	ldi r16, (1 << SRL2)			; SRL2	= 6		Wait State Sector Limit Bit 2
	out	EMCUCR, r16				; set the Extended MCU Control Registe

	ldi	r16, (1 << SRE) | (1 << ISC11) | (1 << SRW10)		
	out	MCUCR, r16				; set the MicroController Control Register


output:
	ldi   r24,0x42
	rcall serial_tx
	rjmp output



; -----------------------------------------------------------------------------------------
; egpio_write
; -----------------------------------------------------------------------------------------
;
; write to the EGPIO memory map with status data
; r16 [6:0] address [7] data 

egpio_write:
	ldi	XH, EGPIO_BASE >> 8		; high byte of EGPIO base location
	mov	XL, r16				; low byte from r16
							; this may have been ORed with the EGPIO_SET
	andi	XL, 0x7F
;	cbr	XL, EGPIO_SET			; clear top bit giving the correct address in X
							; of BASE + Select (0-7)
							; retrieve the data from bit7 which may have been set
	bst	r16, 7				; stores bit7 of r16 in T flag
							; T:0 if bit 7 of r16 is cleared. Set to 1 otherwise.
	eor	r16, r16
;	clr	r16					; clear r16
	bld	r16, 0				; load T to bit 0 of r16, so will have a 0 or 1 value
	st	X, r16				; store r16 in data space location
	ret


; -----------------------------------------------------------------------------------------
; Serial Tx
; -----------------------------------------------------------------------------------------
;
; r16 = byte to send (ASCII code)

serial_tx:
	push	r18					; preserve r18 to the stack
	push 	XL					; X is used in the egpio_write routine
	push	XH
	in	r18, SREG				; read contents of status register
	push	r18					; preserve status register to the stack including interrupt status
	cli						; stop interrupts
	mov	r18, r24				; put the byte to send in r18

	; send start bit
	ldi	r16, EGPIO_TP1			; sends 0 to data area of EGPIO_TP1 
	rcall	egpio_write

	ldi	tmp, 8				; set to loop through the 8 bits in the byte to send

serial_tx_loop:
	ldi	r16, 16				; set a wait loop for 16

serial_tx_wait:
	dec	r16					; minus 1
	brne	serial_tx_wait			; loop if not finished
							; The value to send has to be sent one bit at a time from 
							; right to left
	ror	r18					; move the bits to the right. bit 0-> C flag
	brcc	send_0				; if lowest bit was clear branch to send_0
	ldi	r16, EGPIO_TP1 | EGPIO_SET 	; otherwise next bit to send is 1
	rjmp	send_x

send_0:
	ldi	r16, EGPIO_TP1			; bit to send is 0
	nop
	nop

send_x:
	rcall	egpio_write				; write the bit to the memory map

	dec	tmp					; minus 1 for the outer tx loop that runs through the 8 bits
	brne	serial_tx_loop			; loop if not finished

	ldi	r16, 18				; set up another loop

serial_tx_wait2:
	dec	r16					; minus 1
	brne	serial_tx_wait2			; loop until finished

	; send stop bit
	ldi	r16, EGPIO_TP1 | EGPIO_SET	; in the memory map EGPIO_TP1 = 1
	rcall	egpio_write

	ldi	r16, 100				; set up another wait loop

serial_tx_wait3:
	dec	r16					; minus 1
	brne	serial_tx_wait3			; loop until finished
		
		
	pop	r18					; retrieve SREG from the stack
	out	SREG, r18				; restore SREG
	pop	XH
	pop	XL
	pop	r18					; restore r18 from the stack
	ret


; -----------------------------------------------------------------------------------------
; Serial Tx Hex
; -----------------------------------------------------------------------------------------
;
; Ouput the byte to the serial in Hexadecimal
;
; r16 = byte

serial_tx_hex:
	push 	r16					; save register to the stack    
	lsr	r16					; get the upper nibble first   
	lsr	r16
	lsr	r16
	lsr	r16
	rcall	serial_tx_nibble			; send the first value	
	pop 	r16					; retrieve the saved value
	andi	r16, 0xf				; seperate the lower nibble
serial_tx_nibble:
	cpi	r16, 10				; check the value is 0-9		
	brcc	serial_tx_hex1			; > 10 goto serial_tx_hex1 to add the A-F hex value
	ldi	tmp, 48				; tmp=48, the ASCII value offset from 0 for 0-9
serial_tx_hex2:
	add	r16, tmp				; add to the lower nibble to get the ASCII code
	rjmp	serial_tx				; transmit
serial_tx_hex1:
	ldi	tmp, 55				; tmp=48, the ASCII value offset from 0 for A-F
	rjmp	serial_tx_hex2			; jmp to add and transmit
