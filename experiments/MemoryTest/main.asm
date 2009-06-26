.include "m162def.inc"
.device	ATmega162

.include "adlc.asm"
.include "EGPIO.asm"
.include "CS8900.asm"
.include "IP.asm"

; =======================================================================
; == ATmega162 I/O pins =================================================
; =======================================================================


							;PB0 ADLC_CLCK				Econet Pin4		
							;PB1 ADLC_D1				Econet Pin8
							;PB2 ADLC_D2				Econet Pin9
							;PB3 ADLC_D3				Econet Pin10
							;PB4 ADLC_D4				Econet Pin11
							;PB5 ADLC_D5				Econet Pin12
							;PB6 ADLC_D6				Econet Pin13
							;PB7 ADLC_D7				Econet Pin14
.equ	ADLC_RS1	= $0	;PD0 Register Select 1		Econet Pin6
.equ	ADLC_RnW	= $1	;PD1 Read/Write Control		Econet Pin2
							;PD2 ADLC_INT				Econet Pin1
							;PD3 NotConnected
.equ	ADLC_RS0	= $4	;PD4 Register Select 0		Econet Pin5
							;PD5 NET CLOCK				DS6391 Pin2
							;PD6 nWR
							;PD7 nRD
.equ	ADLC_nCE	= $0	;PE0						Econet Pin3
							;PE1
.equ	ADLC_D0		= $2	;PE2						Econet Pin7


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


.equ	RX_IDLE			= 0
.equ	RX_DATA			= 1
.equ	FRAME_COMPLETE	= 10


jmp	reset
jmp	adlc_irq
rjmp	adlc_access
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

	; set up I/O

	ldi	r16, 0xff			; set ADLC data pins to output
	out	DDRB, r16			; set PortB data direction register. All output.
	sbi	DDRE, ADLC_D0		; set PortE data direction register ADLC_D0 to output

	ldi	r16, 0xf3			; interrupt pins PD2 & PD3 as inputs 11110011=0xF3 
							; the rest as outputs
	out	DDRD, r16			; set PortD data direction register
	ldi	r16, 0xff			; all high
	out	PORTD, r16			; output 1 on all pins of PORTD

	sbi	DDRE, ADLC_nCE		; set PortE data direction register ADLC_nCE to ouput
	sbi	PORTE, ADLC_nCE		; output 1 on ADLC_nCE

	
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


	out	MCUCR, r16			; set the MicroController Control Register




;	jmp egpio_init		; initialise the External I/O

	ldi	r16, EGPIO_ADLC_RESET   			; 3
	rcall	egpio_write
	ldi	r16, EGPIO_STATUS_RED 				; 2
	rcall	egpio_write
	ldi	r16, EGPIO_TP1 | EGPIO_SET 			; 7
	rcall	egpio_write
	ldi	r16, EGPIO_ECONET_RED  				; 0
	rcall	egpio_write
	ldi	r16, EGPIO_ETHER_RESET | EGPIO_SET 	; 6 force high until the chip is ready to be initialised
	rcall	egpio_write
	ldi	r16, EGPIO_ECONET_GREEN				; 1
	rcall	egpio_write
	ldi	r16, EGPIO_STATUS_GREEN  			; 5
	rcall	egpio_write
	ldi	r16, EGPIO_CLOCK_ENABLE | EGPIO_SET	; 4
	rcall	egpio_write


	; zero sram from 0x5000 to 0xFFFF
	ldi	ZH, 0x5				; High byte Z 0x5
	clr	tmp					; clear tmp
	clr	ZL					; clear low byte Z
zero1:						; start loop around SRAM locations
	st	Z, tmp				; store in dataspace Z, 0 
	inc	ZL					; increment low byte of loop
	brne	zero1			; go from 0 to &FF
	inc	ZH					; then increment ZH
	brne	zero1			; until it reaches &FFFF


	rcall	crlf			; output crlf to serial port
	rcall	debug			; debug test output

	; set up interrupt handler
	clr	r18					; 
	out	GICR, r18			; clear the General Interrupt Control Register

	sei						; set interrupts

	; unreset ethernet chip by setting the reset line low
	ldi	r16, EGPIO_ETHER_RESET
	rcall	egpio_write		; set to 0

	; initialise the ADLC
	rcall	adlc_init

test_loop:
	ldi	r16, 0
	rcall	adlc_read
	rjmp	test_loop

	ldi	r18, (1 << INT0)	; enable adlc interrupts
	out	GICR, r18

loop:
	;rcall	cs8900_poll

	lds	r16, adlc_state
	cpi	r16, FRAME_COMPLETE
	breq	adlc_frame
	rjmp	loop

adlc_frame:
	ldi	ZH, ECONET_RX_BUF >> 8
	clr	ZL
	lds	YH, adlc_rx_ptr + 1
	lds	YL, adlc_rx_ptr

	rcall	crlf

	mov	r16, YH
	rcall	serial_tx_hex
	mov	r16, YL
	rcall	serial_tx_hex
	rcall	crlf

	mov	r16, ZH
	rcall	serial_tx_hex
	mov	r16, ZL
	rcall	serial_tx_hex
	rcall	crlf

print_frame_loop:
	ld	r16, Z+
	rcall	serial_tx_hex
	ldi	r16, 32
	rcall	serial_tx
	cp	ZH, YH
	brne	print_frame_loop
	cp	ZL, YL
	brne	print_frame_loop
	rcall	crlf
	rcall	adlc_ready_to_receive
	rjmp	loop



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
	ldi	r16, EGPIO_ADLC_RESET   			; 3
	rcall	egpio_write
	ldi	r16, EGPIO_STATUS_RED 				; 2
	rcall	egpio_write
	ldi	r16, EGPIO_TP1 | EGPIO_SET 			; 7
	rcall	egpio_write
	ldi	r16, EGPIO_ECONET_RED  				; 0
	rcall	egpio_write
	ldi	r16, EGPIO_ETHER_RESET | EGPIO_SET 	; 6 force high until the chip is ready to be initialised
	rcall	egpio_write
	ldi	r16, EGPIO_ECONET_GREEN				; 1
	rcall	egpio_write
	ldi	r16, EGPIO_STATUS_GREEN  			; 5
	rcall	egpio_write
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
; r16 = byte to send

serial_tx:
	push	r18						; preserve r18 to the stack
	in	r18, SREG					; read contents of status register
	push	r18						; preserve status register to the stack including interrupt status
	cli								; stop interrupts
	mov	r18, r16					; put the byte to send in r18

	; send start bit
	ldi	r16, EGPIO_TP1				; sends 0 to data area of EGPIO_TP1 
	rcall	egpio_write

	ldi	tmp, 8						; set to loop through the 8 bits in the byte to send
serial_tx_loop:
	ldi	r16, 16						; set a wait loop for 16
serial_tx_wait:
	dec	r16							; minus 1
	brne	serial_tx_wait			; loop if not finished
									; The value to send has to be sent one bit at a time from 
									; right to left
	ror	r18							; move the bits to the right. bit 0-> C flag
	brcc	send_0					; if lowest bit was clear branch to send_0
	ldi	r16, EGPIO_TP1 | EGPIO_SET  ; otherwise next bit to send is 1
	rjmp	send_x
send_0:
	ldi	r16, EGPIO_TP1				; bit to send is 0
	nop
	nop
send_x:
	rcall	egpio_write				; write the bit to the memory map

	dec	tmp							; minus 1 for the outer tx loop that runs through the 8 bits
	brne	serial_tx_loop			; loop if not finished

	ldi	r16, 18						; set up another loop
serial_tx_wait2:
	dec	r16							; minus 1
	brne	serial_tx_wait2			; loop until finished

	; send stop bit
	ldi	r16, EGPIO_TP1 | EGPIO_SET	; in the memory map EGPIO_TP1 = 1
	rcall	egpio_write

	ldi	r16, 100					; set up another wait loop
serial_tx_wait3:
	dec	r16							; minus 1
	brne	serial_tx_wait3			; loop until finished
		
		
	pop	r18							; retrieve SREG from the stack
	out	SREG, r18					; restore SREG
	pop	r18							; restore r18 from the stack
	ret

; -----------------------------------------------------------------------------------------
; Serial Tx Hex
; -----------------------------------------------------------------------------------------
;
; r16 = byte

serial_tx_hex:
	push 	r16
	lsr	r16
	lsr	r16
	lsr	r16
	lsr	r16
	rcall	serial_tx_nibble
	pop 	r16
	andi	r16, 0xf
serial_tx_nibble:
	cpi	r16, 10
	brcc	serial_tx_hex1
	ldi	tmp, 48
serial_tx_hex2:
	add	r16, tmp
	rjmp	serial_tx
serial_tx_hex1:
	ldi	tmp, 55
	rjmp	serial_tx_hex2

; -----------------------------------------------------------------------------------------
; CRLF
; -----------------------------------------------------------------------------------------
;
;output <cr><lf> to the serial debugger

crlf:
	ldi	r16, 13				; <CR> to r16
	rcall	serial_tx		; ouput to serial
	ldi	r16, 10				; <LF> to r16
	rjmp	serial_tx		; output to serial

; -----------------------------------------------------------------------------------------
; debug
; -----------------------------------------------------------------------------------------
;
; output "debug" to the serial debugger

debug:
	ldi r16, 0x64	
	rcall	serial_tx		; ouput to serial
	ldi r16, 0x65	
	rcall	serial_tx		; ouput to serial
	ldi r16, 0x62			
	rcall	serial_tx		; ouput to serial
	ldi r16, 0x75
	rcall	serial_tx		; ouput to serial
	ldi r16, 0x67
	rjmp	serial_tx		; ouput to serial
