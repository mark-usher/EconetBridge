#define _SFR_ASM_COMPAT 1
#define __SFR_OFFSET 0     /* Required for port labels to work as expected */
#include <avr/io.h>        /* Contains I/O port declarations */


#  define addr_lo		r3
#  define addr_hi		r4
#  define data		r2


//  Make these functions visible to other source files.
.global serial_tx	
.global serial_tx_hex
.global egpio_init


; =======================================================================
; == Serial =============================================================
; =======================================================================

	; 115200bps bit time = 8.68us =~ 70 cycles @ 8MHz



#define	EGPIO_TP1			0x7		// Serial TX Output
#define	EGPIO_SET			0x80		// flag to set
#define	EGPIO_BASE			0xC000
#define	EGPIO_ETHER_RESET		0x6		// Ethernet Reset


.section .text             ; Start program section

egpio_init:
	ldi	r16, EGPIO_ETHER_RESET | EGPIO_SET 	
							; 6 force high until the chip is ready to be initialised
	rcall	egpio_write

	; wait
	rcall delay_65536

	ldi	r16, EGPIO_ETHER_RESET		; unreset chip
	rcall	egpio_write

	rcall delay_65536

	rcall crlf

	ret

; -----------------------------------------------------------------------------------------
; CS8900 delay
; -----------------------------------------------------------------------------------------

delay_65536:
	clr	r16
	clr	r17
delay_loop:
	inc	r17
	brne	delay_loop
	inc	r16
	brne	delay_loop
	ret

; -----------------------------------------------------------------------------------------
; egpio_write
; -----------------------------------------------------------------------------------------
;
; write to the EGPIO memory map with status data
; r16 [6:0] address [7] data 

egpio_write:
	ldi	XH, EGPIO_BASE >> 8		; high byte of EGPIO base location
	mov	XL, r24				; low byte from r16
							; this may have been ORed with the EGPIO_SET
	cbr	XL, EGPIO_SET			; clear top bit giving the correct address in X
							; of BASE + Select (0-7)
							; retrieve the data from bit7 which may have been set
	bst	r24, 7				; stores bit7 of r16 in T flag
							; T:0 if bit 7 of r16 is cleared. Set to 1 otherwise.
	clr	r24					; clear r16
	bld	r24, 0				; load T to bit 0 of r16, so will have a 0 or 1 value
	st	X, r24				; store r16 in data space location
	ret




; -----------------------------------------------------------------------------------------
; Serial Tx
; -----------------------------------------------------------------------------------------
;
; r24 = byte to send (ASCII code)
; 

serial_tx:
	push	r18					; preserve r18 to the stack
	push 	XL					; X is used in the egpio_write routine
	push	XH
	in	r18, SREG				; read contents of status register
	push	r18					; preserve status register to the stack including interrupt status
	cli						; stop interrupts
	
	mov	r18, r24				; put the byte to send in r18

	; send start bit
	ldi	r24, EGPIO_TP1			; sends 0 to data area of EGPIO_TP1 
	rcall	egpio_write

	ldi	r16, 8				; set to loop through the 8 bits in the byte to send

serial_tx_loop:
	ldi	r24, 16				; set a wait loop for 16

serial_tx_wait:
	dec	r24					; minus 1
	brne	serial_tx_wait			; loop if not finished
							; The value to send has to be sent one bit at a time from 
							; right to left
	ror	r18					; move the bits to the right. bit 0-> C flag
	brcc	send_0				; if lowest bit was clear branch to send_0
	ldi	r24, EGPIO_TP1 | EGPIO_SET 	; otherwise next bit to send is 1
	rjmp	send_x

send_0:
	ldi	r24, EGPIO_TP1			; bit to send is 0
	nop
	nop

send_x:
	rcall	egpio_write				; write the bit to the memory map

	dec	r16					; minus 1 for the outer tx loop that runs through the 8 bits
	brne	serial_tx_loop			; loop if not finished

	ldi	r24, 18				; set up another loop

serial_tx_wait2:
	dec	r24					; minus 1
	brne	serial_tx_wait2			; loop until finished

	; send stop bit
	ldi	r24, EGPIO_TP1 | EGPIO_SET	; in the memory map EGPIO_TP1 = 1
	rcall	egpio_write

	ldi	r24, 100				; set up another wait loop

serial_tx_wait3:
	dec	r24					; minus 1
	brne	serial_tx_wait3			; loop until finished
		
		
	pop	r18					; retrieve SREG from the stack
	out	SREG, r18				; restore SREG
	pop	XH
	pop	XL
	pop	r18					; restore r18 from the stack
	ret



; -----------------------------------------------------------------------------------------
; CRLF
; -----------------------------------------------------------------------------------------
;
; output <cr><lf> to the serial debugger

crlf:
	ldi	r24, 13				; <CR> to r16
	rcall	serial_tx				; ouput to serial
	ldi	r24, 10				; <LF> to r16
	rjmp	serial_tx				; output to serial



; -----------------------------------------------------------------------------------------
; Serial Tx Hex
; -----------------------------------------------------------------------------------------
;
; Ouput the byte to the serial in Hexadecimal
;
; r16 = byte

serial_tx_hex:
	push 	r24					; save register to the stack    
	lsr	r24					; get the upper nibble first   
	lsr	r24
	lsr	r24
	lsr	r24
	rcall	serial_tx_nibble			; send the first value	
	pop 	r24					; retrieve the saved value
	andi	r24, 0xf				; seperate the lower nibble
serial_tx_nibble:
	cpi	r24, 10				; check the value is 0-9		
	brcc	serial_tx_hex1			; > 10 goto serial_tx_hex1 to add the A-F hex value
	ldi	r16, 48				; tmp=48, the ASCII value offset from 0 for 0-9
serial_tx_hex2:
	add	r24, r16				; add to the lower nibble to get the ASCII code
	rjmp	serial_tx				; transmit
serial_tx_hex1:
	ldi	r16, 55				; tmp=48, the ASCII value offset from 0 for A-F
	rjmp	serial_tx_hex2			; jmp to add and transmit





; -----------------------------------------------------------------------------------------
; CS8900 WriteIORegister
; -----------------------------------------------------------------------------------------
;
; Write the 16 bit data in r18, r19 to the register, r16, r17.
;
; on entry
; 		X = register
; 		r16/r17 = data
; on exit
; 		X = register+1
; 		r16/r17 = data


.global aWriteIORegister
aWriteIORegister:



	mov	XL, addr_lo
	mov	XH, addr_hi

	st	X, data			; Write the 16 bits of data to the IO Register


/*	rcall	crlf
	// print the address
	ldi	r24, 0x42
	rcall	serial_tx

	ldi	r24, 0x20
	rcall	serial_tx

	mov	r24, addr_hi
	rcall	serial_tx_hex
	mov	r24, addr_lo
	rcall	serial_tx_hex
	
	ldi	r24, 0x20
	rcall	serial_tx
	mov	r24, data
	rcall	serial_tx_hex

	ldi	r24, 0x20
	rcall	serial_tx
*/

	ret



.global aReadIORegister

; -----------------------------------------------------------------------------------------
; CS8900 ReadIORegister
; -----------------------------------------------------------------------------------------
;
; Write the 16 bit data in r18, r19 to the register, r16, r17.
;
; on entry
; 		r16/r17 = addr
; on exit
; 		r16=data


aReadIORegister:
	
	mov	XL, addr_lo
	mov	XH, addr_hi
	ld	data, X
/*
	rcall	crlf
	// print the address
	ldi	r24, 0x41
	rcall	serial_tx

	ldi	r24, 0x20
	rcall	serial_tx

	mov	r24, addr_hi
	rcall	serial_tx_hex
	mov	r24, addr_lo
	rcall	serial_tx_hex
	
	ldi	r24, 0x20
	rcall	serial_tx
*/
	
	ret

