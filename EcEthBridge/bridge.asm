.include "m162def.inc"

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
.equ	ADLC_RS1		= $0			;PD0 Register Select 1			Econet Pin6
.equ	ADLC_RnW		= $1			;PD1 Read/Write Control			Econet Pin2
							;PD2 ADLC_INT				Econet Pin1
							;PD3 NotConnected
.equ	ADLC_RS0		= $4			;PD4 Register Select 0			Econet Pin5
							;PD5 NET CLOCK				DS6391 Pin2
							;PD6 nWR
							;PD7 nRD
.equ	ADLC_nCE		= $0			;PE0						Econet Pin3
							;PE1
.equ	ADLC_D0		= $2			;PE2						Econet Pin7


.def	chksumL		= r0
.def	chksumM		= r1
.def	chksumH		= r2

.def	WL			= r16
.def	WH			= r17

.def	valueL		= r20
.def	valueH		= r21

.def	lengthL		= r26
.def	lengthH		= r27

.def	tmp			= r24

; =======================================================================
; == ADLC ===============================================================
; =======================================================================
;
; The ADLC has four control registers and two status registers.


.equ	ADLC_CR1		= 0
.equ	ADLC_CR2		= 1			; control bit = 0
.equ	ADLC_CR3		= 1 			; control bit = 1
.equ	ADLC_CR4		= 3
.equ	ADLC_SR1		= 0
.equ	ADLC_SR2		= 1

.equ	ADLC_TXCONTINUE	= 2
.equ	ADLC_TXTERMINATE	= 3
.equ	ADLC_RX		= 2

; control register 1
.equ	CR1_AC		= 0x01		; Address Control. Switch controlling access to CR2 or CR3
.equ	CR1_RIE		= 0x02		; Set to enable Rx interrupts
.equ	CR1_TIE		= 0x04		; Set to enable Tx interrupts
.equ	CR1_RDSR_MODE	= 0x08		; -not used (DMA)
.equ	CR1_TDSR_MODE	= 0x10		; -not used (DMA)
.equ	CR1_DISCONTINUE	= 0x20		; -not used Rx Frame Discontinue
.equ	CR1_RXRESET		= 0x40		; Resets all Rx status and FIFO registers
.equ	CR1_TXRESET		= 0x80		; Resets all Tx status and FIFO registers

; control register 2
.equ	CR2_PSE		= 0x01		; Status bit prioritised
.equ	CR2_TWOBYTE		= 0x02		; Should be set. Causes ADLC to work in two byte mode so that
							; Receive Data Available and Transmit Data Register Available are
							; asserted when two data bytes are ready and two transmit registers
							; available
.equ	CR2_FLAGIDLE	= 0x04		; Determines whether flag fill or mark idle is to be used between
							; packet transfers
.equ	CR2_FC		= 0x08		; Select frame complete or Tx data register (TDRA) ready
.equ	CR2_TXLAST		= 0x10		; Transmit last byte. (Initially set to 0; used to indicate when
							; CRC and closing flag should be transmitted.)
.equ	CR2_CLRRX		= 0x20		; When set Rx status bits are cleared
.equ	CR2_CLRTX		= 0x40		; When set Tx status bits are cleared
.equ	CR2_RTS		= 0x80		; Request to send. Controls the RTS output used to enable the
							; output transmit driver.
; control register 3
.equ	CR3_LCF		= 0x01		; Indicates that no logical control field select byte is included in the packet
.equ	CR3_CEX		= 0x02		; Indicates control field is 8 bits long
.equ	CR3_AEX		= 0x04		; No auto address extend; ADLC assumes 8 bit address. (Although
							; Econet uses 15 bit addresses, the ADLC's auto address extend
							; facility is not used since the high order bit has special 
							; significance.)
.equ	CR3_ZERO_ONE	= 0x08		; Idle. (01/11). set to be all ones
.equ	CR3_FDSE		= 0x10		; - not used Flag Detected Status Enable
.equ	CR3_LOOP		= 0x20		; - not used Loop/Non loop mode
.equ	CR3_TST		= 0x40		; - not used Go Active on Poll / test
.equ	CR3_DTR		= 0x80		; - not used Loop On-Line Control DTR

; control register 4
.equ	CR4_DFSF		= 0x01		; Indicates closing flag of one packet is also opening flag of
							; next. (Irrelevant in Econet systems since reception always 
							; follows transmission.)
.equ	CR4_TXWLS		= 0x06  		; Set Tx word length to 8 bits
.equ	CR4_RXWLS		= 0x18		; Set Rx word length to 8 bits
.equ	CR4_ABT		= 0x20		; - not used Transmit Abort
.equ	CR4_ABTEX		= 0x40		; - not used Abort Extend
.equ	CR4_NRZI		= 0x80		; - not used NRZI/NRZ

; status register 1
.equ	SR1_RDA		= 0x01		; Receive data available - set to confirm that two bytes are
							; available in the receive FIFO data buffer
.equ	SR1_S2RQ		= 0x02		; - not used Status #2 Read request
.equ	SR1_LOOP		= 0x04		; - not used Loop
.equ	SR1_FD		= 0x08		; - not used Flag Detected (when enabled)
.equ	SR1_CTS		= 0x10		; Clear to send. This bit reflects the condition of the CTS input.
							; If enable, a switch of CTS from low to high causes an interrupt.
.equ	SR1_TXU		= 0x20		; - not used
.equ	SR1_TDRA		= 0x40		; Tx data register available. This should be read before sending
							; a byte. When CTS is high, TDRA is deasserted to indicate that
							; transmission should end.
.equ	SR1_IRQ		= 0x80		; IRQ. Interrupts are enabled as appropriage to indicate transmit,
							; receive and error conditions.

	; status register 2
.equ	SR2_AP		= 0x01		; Address present. Set to indicate that the first byte of a packet
							; is available for reading.
.equ	SR2_FV		= 0x02		; Frame valid. Indicates packet received and CRC correct.
.equ	SR2_RXIDLE		= 0x04		; Receive idle. Set to cause an IRQ after 15 one bits have been 
							; received.
.equ	SR2_RXABT		= 0x08		; - not used Abort Received
.equ	SR2_ERR		= 0x10		; - not used FCS Error
.equ	SR2_DCD		= 0x20		; Data carrier detect. Reflects the DCD input, showing the state
							; of the clock signal. If enabled, an interrupt is generated if 
							; the clock fails.
.equ	SR2_OVRN		= 0x40		; - not used Rx Overun
.equ	SR2_RDA		= 0x80		; Receive data available. Used after a receive interrupt to confirm
							; that the data is acutally ready. Other causes of an interrupt
							; indicate an error condition.

; initial values for Control Registers
;.equ	CR2val		= CR2_PSE ;| CR2_TWOBYTE

.equ	CR2val		= CR2_CLRTX | CR2_CLRRX | CR2_FLAGIDLE | CR2_TWOBYTE | CR2_PSE



; =======================================================================
; == External GPIO ======================================================
; =======================================================================

.equ	EGPIO_ECONET_RED	= 0x0			; Econet Red LED
.equ	EGPIO_ECONET_GREEN= 0x1			; Econet Green LED
.equ	EGPIO_STATUS_RED	= 0x2			; Status Red LED
.equ	EGPIO_ADLC_RESET	= 0x3			; ADLC Reset
.equ	EGPIO_CLOCK_ENABLE= 0x4			; Econet Clock Enable
.equ	EGPIO_STATUS_GREEN= 0x5			; Status Green LED
.equ	EGPIO_ETHER_RESET	= 0x6			; Ethernet Reset
.equ	EGPIO_TP1		= 0x7			; Serial TX Output

.equ	EGPIO_SET		= 0x80		; flag to set



; =======================================================================
; == Memory Map SRAM ====================================================
; =======================================================================

.equ	STACK_TOP		= 0x200

.equ	adlc_state		= 0x201		; 1 byte
.equ	adlc_rx_ptr		= 0x202		; 2 bytes
.equ	MAC_addr_DA		= 0x204		; 6 bytes Stored in reverse order - Octet 5 - 0 
.equ	MAC_addr_SA		= 0x20A		; 6 bytes Stored in reverse order - Octet 5 - 0 

.equ	adlc_tmp_ptr	= 0x210		; 2 bytes
.equ	ip_packet_ptr	= 0x212		; 2 bytes

.equ	packet_type		= 0x214		; 0214-0215 2 bytes Type IP 0x08 00
.equ	ip_header		= 0x216		; 0216-0216 1 byte  Version :4 Header Length 20 bytes 0x45
							; 0217-0217 1 byte  Differential Services Field 0x00
.equ	ip_TotalLength	= 0x218		; 0218-0219 2 bytes Total length 40 bytes  0x00 0x28
.equ	ip_ID			= 0x21A		; 021A-021B 2 bytes Identification 0x018d
							; 021C-021D 2 byte  Flags 0x00 
							; 021E-021E 1 byte  TTL   64  0x40
							; 021F-021F 1 byte  Protocol UDP 0x11
.equ	ip_chksum		= 0x220		; 0220-0221 2 bytes Header Checksum 0x77 0x26
.equ	ip_src_ip		= 0x222		; 0222-0225 4 bytes Source IP 1.2.128.9  0x01 0x02 0x80 0x09
.equ	ip_dest_ip		= 0x226		; 0226-0229 4 bytes Destination IP 1.2.128.5 0x01 0x02 0x80 0x05
.equ	udp_packet		= 0x22A		; 022A-022B 2 bytes source TCP port 80 00 = 32768
							; 022C-022D 2 bytes remote TCP port 80 00 = 32768
.equ	udp_length		= 0x22E		; 022E-022F 2 bytes length (incl packet header (8 bytes + data) 00 14
.equ	udp_chksum		= 0x230		; 0230-0231 2 bytes checksum E9 B2
.equ	udp_data		= 0x232		; 0232-     x bytes data 05 00 08 00 06 00 00 00 01 00 00 00

; some default values for the above
.equ	ip_type		= 0x08
.equ	ip_version		= 0x45
.equ	ip_DSF		= 0x00
.equ	ip_length		= 0x14		; the length of the data packet must be added to this
.equ	ip_TTL		= 0x40		; TTL
.equ	ip_Protocol		= 0x11		; UDP
.equ	udp_S_Port_MSB	= 0x80
.equ	udp_S_Port_LSB	= 0x80
.equ	udp_D_Port_MSB	= 0x80
.equ	udp_D_Port_LSB	= 0x00
.equ	udp_header_length	= 0x08

.include "config.inc"

.equ	ECONET_RX_BUF	= 0x4000
.equ	ECONET_TX_BUF	= 0x6000

.equ	CS8900_BASE		= 0x8000
.equ	EGPIO_BASE		= 0xc000


.equ	RX_IDLE		= 0
.equ	RX_DATA		= 1
.equ	RX_SCOUT_ACK	= 2
.equ	FRAME_COMPLETE	= 10


jmp	reset
jmp	adlc_irq
rjmp	adlc_access
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
	out	OCR0, r16				; Timer/Counter 0 Output Compare Register

	; select CTC mode, toggle on OC, no prescaler
	ldi	r16, (1 << WGM01) | (1 << COM00) | (1 << CS00)	
							; WGM01	= 3		Waveform Generation Mode 1
							; COM00	= 4		Compare match Output Mode 0
							; CS00	= 0		Clock Select 0
	out	TCCR0, r16				; TCCR0 - Timer/Counter 0 Control Register

	; set up timer 1 to generate Econet clock output
	; select fast PWM with TOP=ICR1
	; clear OC on MATCH, set at TOP
	ldi	r16, (1 << WGM11) | (1 << COM1A1)
							; WGM11	= 1		Pulse Width Modulator Select Bit 1
							; COM1A1	= 7		Compare Output Mode 1A, bit 1
	out	TCCR1A, r16				; TCCR1A - Timer/Counter1 Control Register A

	; no prescaling
	ldi	r16, (1 << WGM13) | (1 << WGM12) | (1 << CS10)	
							; WGM13	= 4		Pulse Width Modulator Select Bit 3
							; WGM12	= 3		Pulse Width Modulator Select Bit 2
							; CS10	= 0		Clock Select1 bit 0
	out	TCCR1B, r16				; TCCR1B - Timer/Counter1 Control Register B

	; set up external memory interface and interrupts
	; select 1 wait state for upper region at 0x8000-0xffff
	ldi r16, 1 << SRL2			; SRL2	= 6		Wait State Sector Limit Bit 2
	out	EMCUCR, r16				; set the Extended MCU Control Registe

	ldi	r16, (1 << SRE) | (1 << ISC11) | (1 << SRW10)		
							;SRE	= 7			External SRAM Enable
							;ISC11	= 3		Interrupt Sense Control 1 bit 1
							;ISC11	ISC10	Mode
							;0		0		trigger on low level
							;0		1		reserved
			;	---------->		;1		0		trigger on falling edge
							;1		1		trigger on rising edge
							;SRW10	= 6		External SRAM Wait State Select


	out	MCUCR, r16				; set the MicroController Control Register

	rcall	egpio_init

	rcall init_vars

	rcall	cs_init


	; zero sram from 0x5000 to 0x7FFF
	ldi	ZH, 0x5				; High byte Z 0x5
	clr	tmp					; clear tmp
	clr	ZL					; clear low byte Z
zero1:						; start loop around SRAM locations
	st	Z, tmp				; store in dataspace Z, 0 
	inc	ZL					; increment low byte of loop
	brne	zero1					; go from 0 to &FF
	inc	ZH					; then increment ZH
	cpi	ZH, 0x80
	brne	zero1					; until it reaches &8000

	rcall	crlf					; output crlf to serial port


	; set up interrupt handler
	clr	r18					; 
	out	GICR, r18				; clear the General Interrupt Control Register

	sei						; set interrupts

	; initialise the ADLC
	rcall	adlc_init


	; check for clock
;	ldi	r16, ADLC_SR2			; set read address to ADLC Status Register2
;	rcall	adlc_read				; read the ADLC Status Register into r17

;	mov	r16, r17
;	rcall serial_tx_hex

;	bst	r17, 5				; stores DCD flag bit of Status register into T
;	brtc	clock_present			; check if bit is clear, else 
;	rcall	NoClock				; print no clock
;clock_present:


	ldi	r18, (1 << INT0)			; enable adlc interrupts
	out	GICR, r18

;	rcall	test_xmit
	
loop:
	rcall	cs_poll

	lds	r16, adlc_state			; check the adlc_state
	cpi	r16, FRAME_COMPLETE		; is the frame complete?
	breq	adlc_frame_completed		; yes, then print it to the screen
	rjmp	loop					


adlc_frame_completed:
	rcall	cs_test_tx				; send it out on ethernet
;	rcall	debug_adlc_SR			; debug output of the ADLC Status registers

;	rcall	output_frame_serial		; send frame to serial debugger
	rcall	adlc_ready_to_receive		; reset to the Rx ready state
	rjmp	loop					; main loop



; Output the completed frame to the serial

output_frame_serial:


	ldi	ZH, ECONET_RX_BUF >> 8		; set ZH with the highbyte of the Econet receive buffer
	ldi	ZL, ECONET_RX_BUF & 0xff	; set XL with the lowbyte of the Econet receive buffer
	lds	YH, adlc_rx_ptr + 1		; put the Rx pointer address in Y
	lds	YL, adlc_rx_ptr

	clr tmp
	
print_frame_loop:
	ld	r16, Z+				; get byte from RxBuffer, and increment buffer address counter
	rcall	serial_tx_hex			; output in hex
	
	ldi	r16, 32				; Space
	rcall	serial_tx				; output
	
	cp	ZH, YH				; pointer = RxBuffer position high byte 
	brne	print_frame_loop			; no, then continue and loop
	
	cp	ZL, YL				; pointer = RxBuffer position low byte
	brne	print_frame_loop			; no, then continue and loop


	rcall	crlf					; finished outputting the Rx buffer contents, printer crlf
	
	ret



; Temporary routine to print out the values stored for the status registers

debug_adlc_SR:

	ldi	r16, 0x53 				; "S"
	rcall	serial_tx
	ldi	r16, 0x20 				; " "
	rcall serial_tx

	ldi	ZH, ECONET_TX_BUF >> 8		; set ZH with the highbyte of the Econet Tx buffer
	ldi	ZL, ECONET_TX_BUF & 0xff	; set XL with the lowbyte of the Econet Tx buffer
	lds	YH, adlc_tmp_ptr + 1		; put the Tx pointer address in Y
	lds	YL, adlc_tmp_ptr

	clr tmp
	
print_sr_loop:
	ld	r16, Z+				; get byte from RxBuffer, and increment buffer address counter
	rcall	serial_tx_hex			; output in hex
	
	ldi	r16, 32				; Space
	rcall	serial_tx				; output
	
	cp	ZH, YH				; pointer = RxBuffer position high byte 
	brne	print_sr_loop			; no, then continue and loop
	
	cp	ZL, YL				; pointer = RxBuffer position low byte
	brne	print_sr_loop			; no, then continue and loop

	rcall	crlf					; finished outputting the Rx buffer contents, printer crlf
	ret						; main loop

econet_start_tx:
	; switch to 1-byte mode, no PSE
	ldi	r16, ADLC_CR2			; set to write to Control Register 2
	clr	r17					; clear all bits
	rcall	adlc_write				; write to Control Register

tx_await_idle:	
	ldi	r16, ADLC_SR2
	rcall	adlc_read
	bst	r17, 2				; 4 ;Idle?
	brbc	6, tx_await_idle

	ldi	XL, ECONET_TX_BUF & 0xff
	ldi	XH, ECONET_TX_BUF >> 8
	mov	YL, XL
	mov	YH, XH
	ldi	r16, 6
	add	YL, r16
	ldi	r20, 0
	rcall	adlc_tx_frame

tx_await_ack:
	ldi	r16, ADLC_SR1
	rcall	adlc_read
	bst	r17, 1
	brbc	6, tx_await_ack
	
	ldi	r16, ADLC_SR2
	rcall	adlc_read
	bst	r17, 0
	brbs	6, tx_got_ap
	bst	r17, 2				; 4 ;Idle?
	brbs	6, tx_saw_idle

tx_error2:
	mov	r16, r17
	rcall	serial_tx_hex
	
	ldi	r16, 0x46
	rjmp	serial_tx

tx_error:
	mov	r16, r17
	rcall	serial_tx_hex
	
	ldi	r16, 0x45
	rjmp	serial_tx

tx_saw_idle:	
	ldi	r16, 0x49
	rjmp	serial_tx

tx_got_ap:
	ldi	r16, ADLC_RX
	rcall	adlc_read

	ldi	XL, ECONET_RX_BUF & 0xff	; set XL with the lowbyte of the Econet receive buffer
	ldi	XH, ECONET_RX_BUF >> 8		; set XH with the highbyte of the Econet receive buffer

	st	X+, r17
	
	sts	adlc_rx_ptr, XL			; set ALDC receive ptr to the start of the Rx buffer
	sts	adlc_rx_ptr + 1, XH

	ldi	r16, RX_SCOUT_ACK
	sts	adlc_state, r16

	ldi	r16, ADLC_CR1
	ldi	r17, CR1_RIE | CR1_TXRESET
	rcall	adlc_write

tx_wait_frame:
	lds	r16, adlc_state
	cpi	r16, FRAME_COMPLETE
	brne	tx_wait_frame

	ldi	XL, ECONET_TX_BUF & 0xff
	ldi	XH, ECONET_TX_BUF >> 8
	mov	YL, ZL
	mov	YH, ZH
	ldi	r20, 1
	rcall	adlc_tx_frame
	
	ret
	

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
	ldi	r16, EGPIO_ADLC_RESET   	; 3
	rcall	egpio_write
	ldi	r16, EGPIO_STATUS_RED 		; 2
	rcall	egpio_write
	ldi	r16, EGPIO_TP1 | EGPIO_SET 	; 7
	rcall	egpio_write
	ldi	r16, EGPIO_ECONET_RED  		; 0
	rcall	egpio_write
	ldi	r16, EGPIO_ETHER_RESET | EGPIO_SET 	
							; 6 force high until the chip is ready to be initialised
	rcall	egpio_write
	ldi	r16, EGPIO_ECONET_GREEN		; 1
	rcall	egpio_write
	ldi	r16, EGPIO_STATUS_GREEN		; 5
	rcall	egpio_write
	ldi	r16, EGPIO_CLOCK_ENABLE | EGPIO_SET	
							; 4
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
	mov	XL, r16				; low byte from r16
							; this may have been ORed with the EGPIO_SET
	cbr	XL, EGPIO_SET			; clear top bit giving the correct address in X
							; of BASE + Select (0-7)
							; retrieve the data from bit7 which may have been set
	bst	r16, 7				; stores bit7 of r16 in T flag
							; T:0 if bit 7 of r16 is cleared. Set to 1 otherwise.
	clr	r16					; clear r16
	bld	r16, 0				; load T to bit 0 of r16, so will have a 0 or 1 value
	st	X, r16				; store r16 in data space location
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
	push	r18					; preserve r18 to the stack
	push 	XL					; X is used in the egpio_write routine
	push	XH
	in	r18, SREG				; read contents of status register
	push	r18					; preserve status register to the stack including interrupt status
	cli						; stop interrupts
	mov	r18, r16				; put the byte to send in r18

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

; -----------------------------------------------------------------------------------------
; CRLF
; -----------------------------------------------------------------------------------------
;
;output <cr><lf> to the serial debugger

crlf:
	ldi	r16, 13				; <CR> to r16
	rcall	serial_tx				; ouput to serial
	ldi	r16, 10				; <LF> to r16
	rjmp	serial_tx				; output to serial

; -----------------------------------------------------------------------------------------
; debug
; -----------------------------------------------------------------------------------------
;
; output "debug" to the serial debugger

debug:
	ldi	r16, 0x64				; "d"
	rcall	serial_tx				; ouput to serial
	ldi	r16, 0x65				; "e"
	rcall	serial_tx				; ouput to serial
	ldi	r16, 0x62				; "b"
	rcall	serial_tx				; ouput to serial
	ldi	r16, 0x75				; "u"
	rcall	serial_tx				; ouput to serial
	ldi	r16, 0x67				; "g"
	rjmp	serial_tx				; ouput to serial

; -----------------------------------------------------------------------------------------
; No Clock
; -----------------------------------------------------------------------------------------
;
; output "No Clock" to the serial debugger
;
noclock:
	ldi	r16, 0x4E				; "N"
	rcall	serial_tx				; ouput to serial
	ldi	r16, 0x6F				; "o"
	rcall	serial_tx				; ouput to serial
	ldi	r16, 0x20				; " "
	rcall	serial_tx				; ouput to serial
	ldi	r16, 0x63				; "c"
	rcall	serial_tx				; ouput to serial
	ldi	r16, 0x6C				; "l"
	rcall	serial_tx				; ouput to serial
	ldi	r16, 0x6F				; "o"
	rcall	serial_tx				; ouput to serial
	ldi	r16, 0x63				; "c"
	rcall	serial_tx				; ouput to serial
	ldi	r16, 0x6B				; "k"
	rcall	crlf					; ouput CRLF



; =======================================================================
; == ADLC ===============================================================
; =======================================================================

test_xmit:	
	ldi	ZH, ECONET_TX_BUF >> 8
	ldi	ZL, ECONET_TX_BUF & 0xff

	;; 0.83 -> 0.254, read fileserver version
	ldi	r16, EconetFS
	st	Z+, r16	
	ldi	r16, EconetNetA
	st	Z+, r16	
	ldi	r16, EconetStation
	st	Z+, r16	
	ldi	r16, EconetNetA
	st	Z+, r16	
	ldi	r16, 0x80
	st	Z+, r16	
	ldi	r16, 0x99
	st	Z+, r16
	ldi	r16, 0x90
	st	Z+, r16
	ldi	r16, 25
	st	Z+, r16
	ldi	r16, 0
	st	Z+, r16
	ldi	r16, 0
	st	Z+, r16
	ldi	r16, 0
	st	Z+, r16
	ldi	r16, 0
	st	Z+, r16
	ldi	r16, 0
	st	Z+, r16

	rjmp	econet_start_tx

.include "adlc.asm"


; =======================================================================
; == IP =================================================================
; =======================================================================


.include "ip.asm"		

; =======================================================================
; == CS8900 =============================================================
; =======================================================================


.include "cs8900.asm"

; =======================================================================
; == Initialise some variables ==========================================
; =======================================================================
;
init_vars:

	; MAC Address stored in reverse Octet 5-0

	;load X with MAC_addr_SA
	ldi	ZL, MAC_addr_SA & 0xff		; set ZL with the lowbyte of the MAC address
	ldi	ZH, MAC_addr_SA >> 8		; set ZH with the highbyte of the MAC address

	clr	r16					; 0x00
	st	Z+,	r16				; Octet 5 of IA

	ldi 	r17,	0x06				; use r17 to save clearing r16 again
	st 	Z+,	r17				; Octet 4 of IA

	ldi	r17,	0x98
	st	Z+,	r17				; Octet 3 of IA
	st	Z+,	r16				; Octet 2 of IA
	st	Z+,	r16				; Octet 1 of IA
	st	Z,	r16				; Octet 0 of IA

	; Store Destination MAC address for testing
	;load Z with MAC_addr_DA
	ldi	ZL, MAC_addr_DA & 0xff		; set ZL with the lowbyte of the MAC address
	ldi	ZH, MAC_addr_DA >> 8		; set ZH with the highbyte of the MAC address

	ldi	r17, MAC_Dest_5 
	st	Z+,	r17				; Octet 5 of IA
	ldi	r17, MAC_Dest_4 
	st 	Z+,	r17				; Octet 4 of IA
	ldi	r17, MAC_Dest_3 
	st	Z+,	r17				; Octet 3 of IA
	ldi	r17, MAC_Dest_2
	st	Z+,	r17				; Octet 2 of IA
	ldi	r17, MAC_Dest_1 
	st	Z+,	r17				; Octet 1 of IA
	ldi	r17, MAC_Dest_0 
	st	Z,	r17				; Octet 0 of IA

	; init IP Header
	clr	r17
	
	ldi	ZL, packet_type & 0xff		; set ZL with the lowbyte of the ip_header structure
	ldi	ZH, packet_type >> 8		; set ZH with the highbyte of the ip_header structure

	ldi	r16, ip_type			; Type IP 0x80 00
	st	Z+, r16
	st	Z+, r17

	ldi	r16, ip_version			; 1 byte  Version :4 Header Length 20 bytes 0x45
	st	Z+, r16

	ldi	r16, ip_DSF				; 1 byte  Differential Services Field 0x00
	st	Z+, r16

	st	Z+, r17				; 2 bytes Total length (currently adds in the 20 bytes for the header)
	ldi	r16, ip_length			; rest to be added ... 40 bytes  0x00 0x28
	st	Z+, r16

	st	Z+, r17				; 2 bytes Identification. Starts at 0 and is increment with each send
	st	Z+, r17

	st	Z+, r17				; 2 byte  Flags
	st	Z+, r17		

	ldi	r16, ip_TTL				; 1 byte  TTL   64  0x40
	st	Z+, r16

	ldi	r16, ip_Protocol			; 1 byte  Protocol UDP 0x11
	st	Z+, r16
						
	st	Z+, r17				; 2 bytes Header Checksum 0x77 0x26
	st	Z+, r17

	ldi	r16, ip_S_Addr_A			; 4 bytes Source IP
	st	Z+, r16
	ldi	r16, ip_S_Addr_B
	st	Z+, r16
	ldi	r16, ip_S_Addr_C
	st	Z+, r16
	ldi	r16, ip_S_Addr_D
	st	Z+, r16

	ldi	r16, ip_D_Addr_A			; 4 bytes Destination IP 
	st	Z+, r16
	ldi	r16, ip_D_Addr_B
	st	Z+, r16
	ldi	r16, ip_D_Addr_C
	st	Z+, r16
	ldi	r16, ip_D_Addr_D
	st	Z+, r16

	; udp packet defaults

	ldi	r16, udp_S_Port_MSB		; 2 bytes source TCP port 80 00 = 32768
	st	Z+, r16
	ldi	r16, udp_S_Port_LSB
	st	Z+, r17

	ldi	r16, udp_D_Port_MSB		; 2 bytes destination TCP port 80 00 = 32768
	st	Z+, r16
	ldi	r16, udp_D_Port_LSB
	st	Z, r16

	ret
