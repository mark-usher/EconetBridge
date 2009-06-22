.include "m162def.inc"
.device	ATmega162


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


.def	chksumL		= r0
.def	chksumM		= r1
.def	chksumH		= r2

.def	WL		= r16
.def	WH		= r17

.def	valueL		= r20
.def	valueH		= r21

.def	lengthL		= r26
.def	lengthH		= r27

.def	tmp		= r24

; =======================================================================
; == ADLC ===============================================================
; =======================================================================
;
; The ADLC has four control registers and two status registers.


.equ	ADLC_CR1		= 0
.equ	ADLC_CR2		= 1		; control bit = 0
.equ	ADLC_CR3		= 1 	; control bit = 1
.equ	ADLC_CR4		= 3
.equ	ADLC_SR1		= 0
.equ	ADLC_SR2		= 1

.equ	ADLC_TXCONTINUE	= 2
.equ	ADLC_TXTERMINATE= 3
.equ	ADLC_RX			= 2

	; control register 1
.equ	CR1_AC			= 0x01	; Address Control. Switch controlling access to CR2 or CR3
.equ	CR1_RIE			= 0x02	; Set to enable Rx interrupts
.equ	CR1_TIE			= 0x04	; Set to enable Tx interrupts
.equ	CR1_RDSR_MODE	= 0x08	; -not used (DMA)
.equ	CR1_TDSR_MODE	= 0x10	; -not used (DMA)
.equ	CR1_DISCONTINUE	= 0x20	; -not used Rx Frame Discontinue
.equ	CR1_RXRESET		= 0x40	; Resets all Rx status and FIFO registers
.equ	CR1_TXRESET		= 0x80	; Resets all Tx status and FIFO registers

	; control register 2
.equ	CR2_PSE			= 0x01	; Status bit prioritised
.equ	CR2_TWOBYTE		= 0x02	; Should be set. Causes ADLC to work in two byte mode so that
								; Receive Data Available and Transmit Data Register Available are
								; asserted when two data bytes are ready and two transmit registers
								; available
.equ	CR2_FLAGIDLE	= 0x04	; Determines whether flag fill or mark idle is to be used between
								; packet transfers
.equ	CR2_FC			= 0x08	; Select frame complete or Tx data register (TDRA) ready
.equ	CR2_TXLAST		= 0x10	; Transmit last byte. (Initially set to 0; used to indicate when
								; CRC and closing flag should be transmitted.)
.equ	CR2_CLRRX		= 0x20	; When set Rx status bits are cleared
.equ	CR2_CLRTX		= 0x40	; When set Tx status bits are cleared
.equ	CR2_RTS			= 0x80	; Request to send. Controls the RTS output used to enable the
								; output transmit driver.
	; control register 3
.equ	CR3_LCF			= 0x01	; Indicates that no logical control field select byte is included in the packet
.equ	CR3_CEX			= 0x02	; Indicates control field is 8 bits long
.equ	CR3_AEX			= 0x04	; No auto address extend; ADLC assumes 8 bit address. (Although
								; Econet uses 15 bit addresses, the ADLC's auto address extend
								; facility is not used since the high order bit has special 
								; significance.)
.equ	CR3_ZERO_ONE	= 0x08	; Idle. (01/11). set to be all ones
.equ	CR3_FDSE		= 0x10	; - not used Flag Detected Status Enable
.equ	CR3_LOOP		= 0x20	; - not used Loop/Non loop mode
.equ	CR3_TST			= 0x40	; - not used Go Active on Poll / test
.equ	CR3_DTR			= 0x80	; - not used Loop On-Line Control DTR

	; control register 4
.equ	CR4_DFSF		= 0x01	; Indicates closing flag of one packet is also opening flag of
								; next. (Irrelevant in Econet systems since reception always 
								; follows transmission.)
.equ	CR4_TXWLS		= 0x06  ; Set Tx word length to 8 bits
.equ	CR4_RXWLS		= 0x18	; Set Rx word length to 8 bits
.equ	CR4_ABT			= 0x20	; - not used Transmit Abort
.equ	CR4_ABTEX		= 0x40	; - not used Abort Extend
.equ	CR4_NRZI		= 0x80	; - not used NRZI/NRZ

	; status register 1
.equ	SR1_RDA			= 0x01	; Receive data available - set to confirm that two bytes are
								; available in the receive FIFO data buffer
.equ	SR1_S2RQ		= 0x02	; - not used Status #2 Read request
.equ	SR1_LOOP		= 0x04	; - not used Loop
.equ	SR1_FD			= 0x08	; - not used Flag Detected (when enabled)
.equ	SR1_CTS			= 0x10	; Clear to send. This bit reflects the condition of the CTS input.
								; If enable, a switch of CTS from low to high causes an interrupt.
.equ	SR1_TXU			= 0x20	; - not used
.equ	SR1_TDRA		= 0x40	; Tx data register available. This should be read before sending
								; a byte. When CTS is high, TDRA is deasserted to indicate that
								; transmission should end.
.equ	SR1_IRQ			= 0x80	; IRQ. Interrupts are enabled as appropriage to indicate transmit,
								; receive and error conditions.

	; status register 2
.equ	SR2_AP			= 0x01	; Address present. Set to indicate that the first byte of a packet
								; is available for reading.
.equ	SR2_FV			= 0x02	; Frame valid. Indicates packet received and CRC correct.
.equ	SR2_RXIDLE		= 0x04	; Receive idle. Set to cause an IRQ after 15 one bits have been 
								; received.
.equ	SR2_RXABT		= 0x08	; - not used Abort Received
.equ	SR2_ERR			= 0x10	; - not used FCS Error
.equ	SR2_DCD			= 0x20	; Data carrier detect. Reflects the DCD input, showing the state
								; of the clock signal. If enabled, an interrupt is generated if 
								; the clock fails.
.equ	SR2_OVRN		= 0x40	; - not used Rx Overun
.equ	SR2_RDA			= 0x80	; Receive data available. Used after a receive interrupt to confirm
								; that the data is acutally ready. Other causes of an interrupt
								; indicate an error condition.

; initial values for Control Registers
;.equ	CR2val			= CR2_PSE ;| CR2_TWOBYTE

.equ	CR2val			= CR2_CLRTX | CR2_CLRRX | CR2_FLAGIDLE | CR2_TWOBYTE | CR2_PSE



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


.equ	ECONET_RX_BUF	= 0x0300
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

	ldi	r16, 0xff				; set ADLC data pins to output
	out	DDRB, r16				; set PortB data direction register. All output.
	sbi	DDRE, ADLC_D0			; set PortE data direction register ADLC_D0 to output

	ldi	r16, 0xf3				; interrupt pins PD2 & PD3 as inputs 11110011=0xF3 
								; the rest as outputs
	out	DDRD, r16				; set PortD data direction register
	ldi	r16, 0xff				; all high
	out	PORTD, r16				; output 1 on all pins of PORTD

	sbi	DDRE, ADLC_nCE			; set PortE data direction register ADLC_nCE to ouput
	sbi	PORTE, ADLC_nCE			; output 1 on ADLC_nCE

	
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

	rcall egpio_init


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

	rcall	crlf				; output crlf to serial port


	; set up interrupt handler
	clr	r18						; 
	out	GICR, r18				; clear the General Interrupt Control Register

	sei							; set interrupts

	; unreset ethernet chip by setting the reset line low
	ldi	r16, EGPIO_ETHER_RESET
	rcall	egpio_write			; set to 0

	; initialise the ADLC
	rcall	adlc_init


	; check for clock
;	ldi	r16, ADLC_SR2			; set read address to ADLC Status Register2
;	rcall	adlc_read			; read the ADLC Status Register into r17

;	mov r16, r17
;	rcall serial_tx_hex

;	bst r17, 5					; stores DCD flag bit of Status register into T
;	brtc clock_present		; check if bit is clear, else 
;	rcall	NoClock				; print no clock
;clock_present:


;test_loop:
;	ldi	r16, 0
;	rcall	adlc_read
;	rjmp	test_loop

	ldi	r18, (1 << INT0)		; enable adlc interrupts
	out	GICR, r18



loop:
	;rcall	cs8900_poll

	lds	r16, adlc_state				; check the adlc_state
	cpi	r16, FRAME_COMPLETE			; is the frame complete?
	breq	adlc_frame				; yes, then print it to the screen
	rjmp	loop					


adlc_frame:
	ldi	ZH, ECONET_RX_BUF >> 8		; set ZH with the highbyte of the Econet receive buffer
	ldi	ZL, ECONET_RX_BUF & 0xff	; set XL with the lowbyte of the Econet receive buffer
	lds	YH, adlc_rx_ptr + 1			; put the Rx pointer address in Y
	lds	YL, adlc_rx_ptr

;	rcall	crlf

;	mov	r16, ZH						; Output in hex the Rx Buffer address
;	rcall	serial_tx_hex
;	mov	r16, ZL
;	rcall	serial_tx_hex
;	ldi r16, 0x2D					; "-"
;	rcall	serial_tx				; output
	
;	mov	r16, YH						; Ouput in hex the Rx Pointer address
;	rcall	serial_tx_hex
;	mov	r16, YL
;	rcall	serial_tx_hex
;	rcall	crlf

	; ignore the pointer and just print the first 10 bytes of the buffer
;	ldi	YH, ECONET_RX_BUF >> 8		; put the Rx pointer address in Y
;	ldi	YL, 0xA


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
	
	rcall	adlc_ready_to_receive	; reset to the Rx ready state
	rjmp	loop					; main loop

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

; -----------------------------------------------------------------------------------------
; debug
; -----------------------------------------------------------------------------------------
;
; output "debug" to the serial debugger

debug:
	ldi r16, 0x64				; "d"
	rcall	serial_tx			; ouput to serial
	ldi r16, 0x65				; "e"
	rcall	serial_tx			; ouput to serial
	ldi r16, 0x62				; "b"
	rcall	serial_tx			; ouput to serial
	ldi r16, 0x75				; "u"
	rcall	serial_tx			; ouput to serial
	ldi r16, 0x67				; "g"
	rjmp	serial_tx			; ouput to serial

; -----------------------------------------------------------------------------------------
; No Clock
; -----------------------------------------------------------------------------------------
;
; output "No Clock" to the serial debugger
;
noclock:
	ldi r16, 0x4E				; "N"
	rcall	serial_tx			; ouput to serial
	ldi r16, 0x6F				; "o"
	rcall	serial_tx			; ouput to serial
	ldi r16, 0x20				; " "
	rcall	serial_tx			; ouput to serial
	ldi r16, 0x63				; "c"
	rcall	serial_tx			; ouput to serial
	ldi r16, 0x6C				; "l"
	rcall	serial_tx			; ouput to serial
	ldi r16, 0x6F				; "o"
	rcall	serial_tx			; ouput to serial
	ldi r16, 0x63				; "c"
	rcall	serial_tx			; ouput to serial
	ldi r16, 0x6B				; "k"
	rcall	crlf				; ouput CRLF


; =======================================================================
; == ADLC ===============================================================
; =======================================================================

; -----------------------------------------------------------------------------------------
; ADLC Read
; -----------------------------------------------------------------------------------------
;
; r16=address
; r17=data

adlc_read:
	cbi	PORTD, ADLC_RS0			; clear RS0 on PORTD
	cbi	PORTD, ADLC_RS1			; clear RS1 on PORTD
	sbrc	r16, 0				; if bit 0 in the address field is not clear
	sbi	PORTD, ADLC_RS0			;    set bit 0
	sbrc	r16, 1				; if bit 1 in the address field is not clear
	sbi	PORTD, ADLC_RS1			;    set bit 1
	sbi	PORTD, ADLC_RnW			; Read from ADLC

	ldi	r16, 0x1				; make sure the ADLC_CLK at PB0 stays set
	out	DDRB, r16				; clear all other lines on PORTB
	cbi	DDRE, ADLC_D0			; clear ADLC_D0 line on PORTE

	clc
	
	in	r18, GICR				; save the contents of the General Interrupt Control Register
	ldi	tmp, (1 << INT1)		; External INT1 enable
	out	GIFR, tmp				; write to the General Interrupt Flag Register
	out	GICR, tmp				; write to the General Interrupt Control Register


adlc_read_wait:					; wait for adlc_access to run
	brcc	adlc_read_wait		; r16 and r17 will return with PortB and PortE
	
	bst	r16, ADLC_D0			; Store the bit from ADLC_D0 in the T flag
	bld	r17, 0					; load T into bit 0 of r17 to leave the byte read in r17

	ldi	r16, 0xff				; set pins back to output
	out	DDRB, r16				; Port B
	sbi	DDRE, ADLC_D0			; ADLC_DO pin on PORT E
	ret


; -----------------------------------------------------------------------------------------
; ADLC Write
; -----------------------------------------------------------------------------------------
;
; r16=address
; r17=data

adlc_write:
	; set the addressing
	cbi	PORTD, ADLC_RS0			; clear RS0 on PORTD -
	cbi	PORTD, ADLC_RS1			; clear RS1 on PORTD - ADLC Rx
	sbrc	r16, 0				; if bit 0 in the address field is not clear
	sbi	PORTD, ADLC_RS0			;    set bit 0
	sbrc	r16, 1				; if bit 1 in the address field is not clear
	sbi	PORTD, ADLC_RS1			;    set bit 1
	cbi	PORTD, ADLC_RnW			; write to ADLC

	; set the data
	out	PORTB, r17				; put the data on PORT B
	cbi	PORTE, ADLC_D0			; clear bit 2 on PORT E
	sbrc	r17, 0				; if bit 0 in the data field is not clear
	sbi	PORTE, ADLC_D0			;    set bit to 1

	clc
	
	in	r18, GICR				; save the contents of the General Interrupt Control Register
	ldi	tmp, (1 << INT1)		; External INT1 enable
	out	GIFR, tmp				; write to the General Interrupt Flag Register
	out	GICR, tmp				; write to the General Interrupt Control Register

adlc_write_wait:				; wait for adlc_access to run
	brcc adlc_write_wait		; by checking if carry is cleared
	ret



; -----------------------------------------------------------------------------------------
; ADLC access
; -----------------------------------------------------------------------------------------
;
; perform one read or write access cycle

adlc_access:
	nop
	nop
	cbi	PORTE, ADLC_nCE			; Clear the ADLC_nCE
	nop	
	nop
	nop
	nop
	nop
	nop
	in	r17, PINB				; read PortB to r17
	in	r16, PINE				; read PortE to r16
	sbi	PORTE, ADLC_nCE			; set ADLC_nCE to flag read completed
	out	GICR, r18				; restore the contents of the Global Interrupt Control Register
	sec							; set carry to flag interrupt is handled
	reti						; return from interrupt

; -----------------------------------------------------------------------------------------
; ADLC Tx frame
; -----------------------------------------------------------------------------------------
;

adlc_tx_frame:
	ret							; do nothing

; -----------------------------------------------------------------------------------------
; ADLC Tx IRQ
; -----------------------------------------------------------------------------------------
;

adlc_tx_irq:
	rjmp	adlc_irq_ret		; return from interrupt


; -----------------------------------------------------------------------------------------
; ADLC process_fv Frame Valid
; -----------------------------------------------------------------------------------------
;
;
; exit : adlc_state = FRAME COMPLETE
; 
process_fv:
	ldi	r16, 0x76				; "v" CRC is  Valid
	call	serial_tx			; write to serial

	rcall	adlc_rx_flush		; clear remaining bytes in the input buffer

	ldi	r16, FRAME_COMPLETE		;
	sts	adlc_state, r16			; set ADLC state to FRAME COMPLETE

	ldi	r16, ADLC_CR1			; write to Control Register 1
	ldi	r17, CR1_TXRESET		; Tx Reset
	rcall	adlc_write			; write to ADLC

	rjmp	adlc_irq_ret		; return from interrupt



; -----------------------------------------------------------------------------------------
; ADLC IRQ
; -----------------------------------------------------------------------------------------
;

adlc_irq:	
	push	tmp					; save register values to the stack
	push	r16
	push	r17
	in		r16, SREG			; Save the status register
	push	r16

	; disable adlc irq and re-enable global interrupts
	clr	tmp
	out	GICR, tmp
	sei

	lds	tmp, adlc_state			; get the ADLC state
	;bst	tmp, 7				; bit store bit 7 in of tmp in T
	;brbs	6, adlc_tx_irq		; if T is set, ADLC state is Tx - branch to adlc_tx_irq

	ldi	r16, ADLC_SR1			; set read address to ADLC Status Register1
	rcall	adlc_read			; read the ADLC Status Register into r17

	bst	r17, 0					; RDA? Receive data available
	brbs	6, process_rda		; yes
	bst	r17, 1					; S2RQ - Status #2 Read Request ?
	brbs	6, process_s2rq		; yes

	ldi	r16, 0x56				; "V" - valid CRC is Valid
	rcall	serial_tx			; write to serial

	rjmp	adlc_irq_ret		; clean up routine for interrupt handling


process_s2rq:
	ldi	r16, ADLC_SR2			; set address to Status Register2
	rcall	adlc_read			; read Status Register2
	bst	r17, 0					; 1 ;AP: Address Present?
	brbs	6, process_ap		;   ;yes
	bst	r17, 7					; 80;RDA? Receive Data Available
	brbs	6, process_rda		;   ;yes
	bst	r17, 6					; 40;overrun? Rx Overrun
	brbs	6, rx_overrun		;   ;yes
	bst	r17, 5					; 20;DCD? signifies presence of clock
	brbs	6, process_dcd		;   ;yes
	bst	r17, 4					; 10;CRC error?
	brbs	6, rx_error			;   ;yes
	bst	r17, 3					; 8 ;Abort? Abort Received?
	brbs	6, abandon_rx		;   ;yes
	bst	r17, 2					; 4 ;Idle?: Receive Idle?
	brbs	6, process_idle		;   ;yes
	bst	r17, 1					; 2 ;FV: Frame Valid?
	brbs	6, process_fv		;   ;yes


; -----------------------------------------------------------------------------------------
; ADLC process_ap Address Present
; -----------------------------------------------------------------------------------------
;
; The address field is the first 8 bits following the opening flag, so will always be the
; first byte of the packet. This byte being present causes the Rx Interrupt if enabled.

process_ap:
;	ldi	r16, 0x61				; "a"
;	call	serial_tx			; send to serial

	lds	XL, adlc_rx_ptr			; put the Rx buffer pointer address in X
	lds	XH, adlc_rx_ptr + 1

	; Read 1 byte to the buffer
	ldi	r16, ADLC_RX			; set read address to ADLC_RX (2) 
	rcall	adlc_read			; read ADLC, r17 = data read

	st	X+, r17					; store the data read in the Rx buffer

	sts	adlc_rx_ptr, XL			; store the incremented Rx buffer pointer
	sts	adlc_rx_ptr + 1, XH		; 

	lds	r16, adlc_state			; add 1 to adlc_state
	inc	r16
	sts	adlc_state, r16

;delay	
	ldi		r16, 160			; set up delay loop
delay:
	dec		r16					; minus 1
	brne	delay				; loop until finished

	rjmp	process_s2rq		; continue processing Status Register 2 until finished


; -----------------------------------------------------------------------------------------
; ADLC process_rda Receive Data Available
; -----------------------------------------------------------------------------------------
;

process_rda:
;	ldi		r16, 0x72			; "r" - rda entered
;	rcall 	serial_tx			; 

	lds	XL, adlc_rx_ptr		; put the Rx buffer pointer address in X
	lds	XH, adlc_rx_ptr + 1

	; read 2 bytes to the buffer

	; 1st byte
	ldi	r16, ADLC_RX			; set read address to ADLC_RX (2) 
	rcall	adlc_read			; read ADLC, r17 = data read
	st	X+, r17					; store the data read in the Rx buffer

	; 2nd byte
	ldi	r16, ADLC_RX			; set read address to ADLC_RX (2) 
	rcall	adlc_read			; read ADLC, r17 = data read
	st	X+, r17					; store the data read in the Rx buffer

	sts	adlc_rx_ptr, XL			; store the incremented Rx buffer pointer
	sts	adlc_rx_ptr + 1, XH		; 

	rjmp	adlc_irq_ret		; return from interrupt



; -----------------------------------------------------------------------------------------
; ADLC rx_overrun
; -----------------------------------------------------------------------------------------
;
rx_overrun:
	ldi		r16, 0x6F			; "o" - overrun
	rcall 	serial_tx			; 
	rjmp 	abandon_rx

; -----------------------------------------------------------------------------------------
; ADLC process_dcd - No Clock
; -----------------------------------------------------------------------------------------
;
process_dcd:
	ldi		r16, 0x6E			; "n" - no clock
	rcall 	serial_tx			; 
;	rcall 	noclock
	rjmp	adlc_irq_ret		; return from interrupt


; -----------------------------------------------------------------------------------------
; ADLC rx_error	Receive Error
; -----------------------------------------------------------------------------------------
;
rx_error:
	ldi		r16, 0x65			; "e" - error
	rcall 	serial_tx			; 
	rcall	adlc_rx_flush		; clear any remaing bytes

; -----------------------------------------------------------------------------------------
; ADLC abandon_rx Abandon Rx
; -----------------------------------------------------------------------------------------
;
; exit: adlc_state = 0
; 
abandon_rx:
	; abandon in-progress reception
	clr	tmp
	sts	adlc_state, tmp			; clear the adlc_state
	rcall	adlc_clear_rx		; clear rx status
	rjmp	process_s2rq		; might be AP as well

; -----------------------------------------------------------------------------------------
; ADLC process_idle
; -----------------------------------------------------------------------------------------
;
; exit: adlc_state = 0
; 
process_idle:					
	ldi	r16, 0x69				; append "i" - network is idle
	call	serial_tx			; write to serial
	rcall	crlf

	clr	tmp						; tmp=0
	sts	adlc_state, tmp			; ADLC state=0

	rcall	adlc_clear_rx		; clear Rx Status Register
	rjmp	adlc_irq_ret		; clean up routine for interrupt handling



; -----------------------------------------------------------------------------------------
; ADLC Rx Flush
; -----------------------------------------------------------------------------------------
	
adlc_rx_flush:
	; switch to 1-byte mode, no PSE
	ldi	r16, ADLC_CR2			; set to write to Control Register 2
	clr	r17						; clear all bits
	rcall	adlc_write			; write to Control Register

	; read SR2
	ldi	r16, ADLC_SR2			; read Status Register 2
	rcall	adlc_read			; read Satus Register into r17

	;if there is a byte to read, read it first before clearing
	
	bst	r17, 7					; RDA? is receive data available
	brbc	6, no_rda			; no

	lds	XL, adlc_rx_ptr			; put the Rx buffer pointer address in X
	lds	XH, adlc_rx_ptr + 1

	; Read 1 byte to the buffer
	ldi	r16, ADLC_RX			; set read address to ADLC_RX (2) 
	rcall	adlc_read			; read ADLC, r17 = data read

	st	X+, r17					; store the data read in the Rx buffer

	sts	adlc_rx_ptr, XL			; store the incremented Rx buffer pointer
	sts	adlc_rx_ptr + 1, XH		; 


no_rda:
	rjmp	adlc_clear_rx		; clear rx status


; -----------------------------------------------------------------------------------------
; ADLC clear Rx
; -----------------------------------------------------------------------------------------
;
; clear rx status and switch back to 1-byte mode
;

adlc_clear_rx:
	ldi	r16, ADLC_CR2			; set to write to Control Register 2
	ldi	r17, CR2val				; reset Receieve status bits
	rjmp	adlc_write			; write to ADLC

; -----------------------------------------------------------------------------------------
; ADLC IRQ ret
; -----------------------------------------------------------------------------------------

adlc_irq_ret:
	ldi	tmp, (1 << INT0)		; restore the interrupt flag
	out	GICR, tmp
	pop	r16						; restore Status register from the Stack
	out	SREG, r16		
	pop	r17						; restore other registers from the Stack
	pop	r16
	pop	tmp
	reti						; return from interrupt

; -----------------------------------------------------------------------------------------
; ADLC init
; -----------------------------------------------------------------------------------------


adlc_init:
	; During a power-on sequence, the ADLC is reset via the RESET input and internally 
	; latched in a reset condition to prevent erroneous output transitions. The four control
	; registers must be programmed prior to the release of the reset condition. The release
	; of the reset condition is peformed by software by writing a "0" into the Rx RS control
	; bit (receiver) and/or Tx RS control bit (transmitter). The release of the reset condition
	; must be done after the RESET input has gone high.
	; At any time during operation, writing a "1" into the Rx RS control bit or Tx RS control
	; bit causes the reset condition of the receiver or the transmitter.


	; raise the reset line on the ADLC
	ldi	r16, EGPIO_ADLC_RESET | EGPIO_SET		; set the ADLC reset to 1
	rcall	egpio_write							; write to ADLC

	; set the initial values of Control Register 1
	ldi	r17, CR1_AC | CR1_TXRESET | CR1_RXRESET	; Switch controlling access to CR3
												; Resets all Tx, Rx status and FIFO registers
												; these will automatically go back to 0
	ldi	r16, ADLC_CR1							; set to ADLC Control Register 1
	rcall	adlc_write							; write to ADLC

	;new
	ldi r17, 0x1E								
	ldi r16, ADLC_TXTERMINATE					; write to TXTerminate address
	rcall adlc_write
	;end new

	; set the initial values of Control Register 3 (CR1_AC previously set)
	clr	r17										; Set initial value to 0
	ldi	r16, ADLC_CR3							; set to ADLC Control Register 3
	rcall	adlc_write							; write to ADLC


	; set the initial values of Control Register 4
	ldi	r17, CR4_TXWLS | CR4_RXWLS				; Set Tx & Rx Word lengths to 8 bits
	ldi	r16, ADLC_CR4							; set to ADLC Control Register 4
	rcall	adlc_write							; write to ADLC


; -----------------------------------------------------------------------------------------
; ADLC ready to receive
; -----------------------------------------------------------------------------------------
;
; exit	: adlc_state = 0
; 		: adlc_rx_ptr    = ECONET RX Buffer LSB
; 		: adlc_rx_ptr + 1    = ECONET RX Buffer MSB

adlc_ready_to_receive:
 	
	;initialise the adlc_state
	
	clr	tmp							; tmp = 0
	sts	adlc_state, tmp				; adlc_state=0

	; initialise the Rx Buffer pointer

	ldi	XL, ECONET_RX_BUF & 0xff	; set XL with the lowbyte of the Econet receive buffer
	ldi	XH, ECONET_RX_BUF >> 8		; set XH with the highbyte of the Econet receive buffer
	
	sts	adlc_rx_ptr, XL				; set ALDC receive ptr to the start of the Rx buffer
	sts	adlc_rx_ptr + 1, XH
	
	ldi	r17, CR1_RIE | CR1_TXRESET	; Enable Receive interrupts | Reset the TX status
	ldi	r16, ADLC_CR1				; set to write to Control Register 1
	rcall	adlc_write				; write to the ADLC

	ldi	r17, CR2val					; Status bit prioritised
	ldi	r16, ADLC_CR2				; set to write to Control Register 2
	rjmp	adlc_write				; write to the ADLC



; =======================================================================
; == CS8900==============================================================
; =======================================================================

	; r16/r17 = address
	; r18/r19 = data
cs8900_write_pp:
	sts	CS8900_BASE + 0xa, r16
	sts	CS8900_BASE + 0xb, r17
	sts	CS8900_BASE + 0xc, r18
	sts	CS8900_BASE + 0xd, r19
	ret

cs8900_read_pp:
	sts	CS8900_BASE + 0xa, r16
	sts	CS8900_BASE + 0xb, r17
	lds	r18, CS8900_BASE + 0xc
	lds	r19, CS8900_BASE + 0xd
	ret


; =======================================================================
; == IP =================================================================
; =======================================================================

;
; Calc checksum. Reads packet at offset zL/zH for
; lengthH/lengthL bytes and calculates the checksum
; in valueH/valueL.
;
cksum: 
	clr     chksumL					; we do the arithmetic
	clr     chksumM					; using a 24
	clr     chksumH					; bit area
	sbrs    lengthL,0				; odd length?
	rjmp	cksumC
	mov	YL,ZL
	mov	YH,ZH
	add	YL,lengthL
	adc	YH,lengthH
    	clr	WL
	st	y,WL		      		 	; clear byte after last
	adiw	lengthL,1
cksumC: 
	lsr	lengthH
    	ror	lengthL
cksuml: 		
	ld      WH,z+		 			; get high byte of 16-bit word
	ld      WL,z+
	add     chksumL,WL	      		; add to accum
	brcc    noLcarry
	ldi     WL,1
	add     chksumM,WL
	brcc    noLcarry
	add     chksumH,WL
noLcarry:
	add     chksumM,WH	      		; add in the high byte
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
	add     chksumL,chksumH	 		; add in the third byte of 24 bit area
	brcc    CkDone1
	inc     chksumM
CkDone1:
	mov     valueL,chksumL
	com	valueL
	mov     valueH,chksumM
	com	valueH
	ret
