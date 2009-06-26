.include "m162def.inc"
.device	ATmega162

.def	tmp		= r24

; =======================================================================
; == External GPIO ======================================================
; =======================================================================

.equ	EGPIO_ECONET_RED	= 0x0		; Econet Red LED
.equ	EGPIO_ECONET_GREEN	= 0x1		; Econet Green LED
.equ	EGPIO_STATUS_RED	= 0x2		; Status Red LED
.equ	EGPIO_ADLC_RESET	= 0x3		; ADLC Reset
.equ	EGPIO_CLOCK_ENABLE	= 0x4		; Econet Clock Enable
.equ	EGPIO_STATUS_GREEN	= 0x5		; Status Green LED
.equ	EGPIO_ETHER_RESET	= 0x6		; Ethernet Reset
.equ	EGPIO_TP1			= 0x7		; Serial TX Output

.equ	EGPIO_SET			= 0x80		; flag to set



; =======================================================================
; == Memory Map SRAM ====================================================
; =======================================================================

.equ	STACK_TOP		= 0x200

.equ	adlc_state		= 0x201
.equ	adlc_rx_ptr		= 0x202


.equ	ECONET_RX_BUF	= 0x4000
.equ	ECONET_TX_BUF	= 0x6000

.equ	CS8900_BASE		= 0x8000
.equ	EGPIO_BASE		= 0xc000



jmp	reset
nop


; =======================================================================
; == Initialisation =====================================================
; =======================================================================

reset:

	; First set up some of the I/O Registers

	; select 8MHz clock
	; CLKPR is Memory mapped and cannot be used with in/out instructions
	ldi	r16, 0x80
	sts	CLKPR, r16
	clr	r16
	sts	CLKPR, r16
 
	; set up stack pointer
	ldi	r16, (STACK_TOP & 0xff)
	out	SPL, r16	
	ldi	r16, (STACK_TOP >> 8)
	out	SPH, r16

	; set up timer 0 to generate ADLC clock waveform
	; select divide-by-10 for nominal 800kHz at 8MHz in
	ldi	r16, 4
	out	OCR0, r16			; Timer/Counter 0 Output Compare Register

	; select CTC mode, toggle on OC, no prescaler
	ldi	r16, (1 << WGM01) | (1 << COM00) | (1 << CS00)	; WGM01	= 3	; Waveform Generation Mode 1
														; COM00	= 4	; Compare match Output Mode 0
														; CS00	= 0	; Clock Select 0
	out	TCCR0, r16										; TCCR0 - Timer/Counter 0 Control Register

	; set up timer 1 to generate Econet clock output
	; select fast PWM with TOP=ICR1
	; clear OC on MATCH, set at TOP
	ldi	r16, (1 << WGM11) | (1 << COM1A1)				; WGM11	= 1	; Pulse Width Modulator Select Bit 1
														; COM1A1	= 7	; Compare Output Mode 1A, bit 1
	out	TCCR1A, r16										; TCCR1A - Timer/Counter1 Control Register A

	; no prescaling
	ldi	r16, (1 << WGM13) | (1 << WGM12) | (1 << CS10)	; WGM13	= 4	; Pulse Width Modulator Select Bit 3
														; WGM12	= 3	; Pulse Width Modulator Select Bit 2
														; CS10	= 0	; Clock Select1 bit 0
	out	TCCR1B, r16										; TCCR1B - Timer/Counter1 Control Register B



	; set up external memory interface and interrupts
	; select 1 wait state for upper region at 0x8000-0xffff
	ldi r16, 1 << SRL2			; SRL2	= 6	; Wait State Sector Limit Bit 2
	out	EMCUCR, r16				; set the Extended MCU Control Registe

	ldi	r16, (1 << SRE) | (1 << ISC11) | (1 << SRW10)		;SRE	= 7	; External SRAM Enable
															;ISC11	= 3	; Interrupt Sense Control 1 bit 1

															;ISC11	ISC10	Mode
															;0		0		trigger on low level
															;0		1		reserved
										;	---------->		;1		0		trigger on falling edge
															;1		1		trigger on rising edge

															;SRW10	= 6	; External SRAM Wait State Select


	out	MCUCR, r16				; set the MicroController Control Register


	; zero sram from 0x5000 to 0xFFFF
	ldi	ZH, 0x5					; High byte Z 0x5
	clr	tmp						; clear tmp
	clr	ZL						; clear low byte Z
zero1:							; start loop around SRAM locations
	st	Z, tmp					; store in dataspace Z, 0 
	inc	ZL						; increment low byte of loop
	brne	zero1				; go from 0 to &FF
	inc	ZH						; then increment ZH
	brne	zero1				; until it reaches &FFFF


	ldi r16, 0x45
	rcall serial_tx
	rcall	crlf				; output crlf to serial port


adlc_frame:
	ldi	ZH, ECONET_RX_BUF >> 8		; set ZH with the highbyte of the Econet receive buffer
	ldi	ZL, ECONET_RX_BUF & 0xff	; set ZL with the lowbyte of the Econet receive buffer

	; ignore the pointer and just print the first 10 bytes of the buffer
	ldi	YH, ECONET_RX_BUF >> 8		; put the Rx pointer address in Y
	ldi	YL, 0x1A


	rcall	crlf

	mov	r16, ZH						; Output in hex the Rx Buffer address
	rcall	serial_tx_hex
	mov	r16, ZL
	rcall	serial_tx_hex
	ldi r16, 0x2D					; "-"
	rcall	serial_tx				; output
	
	mov	r16, YH						; Ouput in hex the Rx Pointer address
	rcall	serial_tx_hex
	mov	r16, YL
	rcall	serial_tx_hex
	rcall	crlf

	clr tmp
	
print_frame_loop:
	ld	r16, Z+						; get byte from RxBuffer, and increment buffer address counter
	rcall	serial_tx_hex			; output in hex
	
	ldi	r16, 32						; Space
	rcall	serial_tx				; output
	
	cp	ZH, YH						; pointer = RxBuffer position high byte 
	brne	print_frame_loop		; no, then continue and loop
	
	cp	ZL, YL						; pointer = RxBuffer position low byte
	brne	print_frame_loop		; no, then continue and loop


	rcall	crlf					; finished outputting the Rx buffer contents, printer crlf
	
	;	reset the start address
	ldi	ZH, ECONET_RX_BUF >> 8		; set ZH with the highbyte of the Econet receive buffer
	ldi	ZL, ECONET_RX_BUF & 0xff	; set ZL with the lowbyte of the Econet receive buffer

	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	rjmp	print_frame_loop					; main loop

; =======================================================================
; == EGPIO ==============================================================
; =======================================================================

; -----------------------------------------------------------------------------------------
; EGPIO init
; -----------------------------------------------------------------------------------------
;
; initialise EGPIOs in the memory map with data setting
; EGPIO base address + value 0-7, data = EGPIO_SET 0 or 1

egpio_init:
;	ldi	r16, EGPIO_ADLC_RESET   			; 3
;	rcall	egpio_write
;	ldi	r16, EGPIO_STATUS_RED 				; 2
;	rcall	egpio_write
	ldi	r16, EGPIO_TP1 | EGPIO_SET 			; 7
	rcall	egpio_write
;	ldi	r16, EGPIO_ECONET_RED  				; 0
;	rcall	egpio_write
	ldi	r16, EGPIO_ETHER_RESET | EGPIO_SET 	; 6 force high until the chip is ready to be initialised
	rcall	egpio_write
;	ldi	r16, EGPIO_ECONET_GREEN				; 1
;	rcall	egpio_write
;	ldi	r16, EGPIO_STATUS_GREEN  			; 5
;	rcall	egpio_write
	ldi	r16, EGPIO_CLOCK_ENABLE | EGPIO_SET	; 4
	rcall	egpio_write
	ret


; -----------------------------------------------------------------------------------------
; egpio_write
; -----------------------------------------------------------------------------------------
;
; write to the EGPIO memory map with status data
; r16 [6:0] address [7] data 

egpio_write:
	ldi	XH, EGPIO_BASE >> 8		; high byte of EGPIO base location
	mov	XL, r16					; low byte from r16
								; this may have been ORed with the EGPIO_SET
	cbr	XL, EGPIO_SET			; clear top bit giving the correct address in X
								; of BASE + Select (0-7)
								; retrieve the data from bit7 which may have been set
	bst	r16, 7					; stores bit7 of r16 in T flag
								; T:0 if bit 7 of r16 is cleared. Set to 1 otherwise.
	clr	r16						; clear r16
	bld	r16, 0					; load T to bit 0 of r16, so will have a 0 or 1 value
	st	X, r16					; store r16 in data space location
	ret




; =======================================================================
; == Serial =============================================================
; =======================================================================

	; 115200bps bit time = 8.68us =~ 70 cycles @ 8MHz

; -----------------------------------------------------------------------------------------
; Serial Tx
; -----------------------------------------------------------------------------------------
;
; r16 = byte to send (ASCII code)

serial_tx:
	push	r18						; preserve r18 to the stack
	in		r18, SREG				; read contents of status register
	push	r18						; preserve status register to the stack including interrupt status
	cli								; stop interrupts
	mov		r18, r16				; put the byte to send in r18

	; send start bit
	ldi		r16, EGPIO_TP1			; sends 0 to data area of EGPIO_TP1 
	rcall	egpio_write

	ldi		tmp, 8					; set to loop through the 8 bits in the byte to send

serial_tx_loop:
	ldi		r16, 16					; set a wait loop for 16

serial_tx_wait:
	dec		r16						; minus 1
	brne	serial_tx_wait			; loop if not finished
									; The value to send has to be sent one bit at a time from 
									; right to left
	ror		r18						; move the bits to the right. bit 0-> C flag
	brcc	send_0					; if lowest bit was clear branch to send_0
	ldi		r16, EGPIO_TP1 | EGPIO_SET  ; otherwise next bit to send is 1
	rjmp	send_x

send_0:
	ldi		r16, EGPIO_TP1			; bit to send is 0
	nop
	nop

send_x:
	rcall	egpio_write				; write the bit to the memory map

	dec		tmp						; minus 1 for the outer tx loop that runs through the 8 bits
	brne	serial_tx_loop			; loop if not finished

	ldi		r16, 18					; set up another loop

serial_tx_wait2:
	dec		r16						; minus 1
	brne	serial_tx_wait2			; loop until finished

	; send stop bit
	ldi		r16, EGPIO_TP1 | EGPIO_SET	; in the memory map EGPIO_TP1 = 1
	rcall	egpio_write

	ldi		r16, 100				; set up another wait loop

serial_tx_wait3:
	dec		r16						; minus 1
	brne	serial_tx_wait3			; loop until finished
		
		
	pop		r18						; retrieve SREG from the stack
	out		SREG, r18				; restore SREG
	pop		r18						; restore r18 from the stack
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
	lsr	r16						; get the upper nibble first   
	lsr	r16
	lsr	r16
	lsr	r16
	rcall	serial_tx_nibble	; send the first value	
	pop 	r16					; retrieve the saved value
	andi	r16, 0xf			; seperate the lower nibble
serial_tx_nibble:
	cpi	r16, 10					; check the value is 0-9		
	brcc	serial_tx_hex1		; > 10 goto serial_tx_hex1 to add the A-F hex value
	ldi	tmp, 48					; tmp=48, the ASCII value offset from 0 for 0-9
serial_tx_hex2:
	add	r16, tmp				; add to the lower nibble to get the ASCII code
	rjmp	serial_tx			; transmit
serial_tx_hex1:
	ldi	tmp, 55					; tmp=48, the ASCII value offset from 0 for A-F
	rjmp	serial_tx_hex2		; jmp to add and transmit

; -----------------------------------------------------------------------------------------
; CRLF
; -----------------------------------------------------------------------------------------
;
;output <cr><lf> to the serial debugger

crlf:
	ldi	r16, 13				; <CR> to r16
	rcall	serial_tx			; ouput to serial
	ldi	r16, 10				; <LF> to r16
	rjmp	serial_tx			; output to serial
