.include "m162def.inc"
.device	ATmega162

; =======================================================================
; == ADLC ===============================================================
; =======================================================================
;
; The ADLC has four control registers and two status registers.


.equ	ADLC_CR1		= 0
.equ	ADLC_CR2		= 1
.equ	ADLC_CR3		= 1
.equ	ADLC_CR4		= 3
.equ	ADLC_SR1		= 0
.equ	ADLC_SR2		= 1

.equ	ADLC_TXCONTINUE	= 2
.equ	ADLC_TXTERMINATE= 3
.equ	ADLC_RX			= 2

	; control register 1
.equ	CR1_AC			= 0x01	; Address Control. Switch controlling access to CR3 or CR4
.equ	CR1_RIE			= 0x02	; Set to enable Rx interrupts
.equ	CR1_TIE			= 0x04	; Set to enable Tx interrupts
.equ	CR1_RDSR_MODE	= 0x08	; -not used (DMA)
.equ	CR1_TDSR_MODE	= 0x10	; -not used (DMA)
.equ	CR1_DISCONTINUE	= 0x20	; -not used Rx Frame Discontinue
.equ	CR1_RXRESET		= 0x40	; Resets all Rx status and FIFO registers
.equ	CR1_TXRESET		= 0x80	; Resets all Tx status and FIFO registers

	; control register 2
.equ	CR2_PSE			= 0x01	; Status bit priotirised
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
.equ	CR2val			= CR2_PSE ;| CR2_TWOBYTE




; -----------------------------------------------------------------------------------------
; ADLC Read
; -----------------------------------------------------------------------------------------
;
; r16=address
; r17=data

adlc_read:
	cbi	PORTD, ADLC_RS0		; clear RS0 on PORTD
	cbi	PORTD, ADLC_RS1		; clear RS1 on PORTD
	sbrc	r16, 0			; if bit 0 in the address field is not clear
	sbi	PORTD, ADLC_RS0		;    set bit 0
	sbrc	r16, 1			; if bit 1 in the address field is not clear
	sbi	PORTD, ADLC_RS1		;    set bit 1
	sbi	PORTD, ADLC_RnW		; Read from ADLC

	ldi	r16, 0x1			; make sure the ADLC_CLK at PB0 stays set
	out	DDRB, r16			; clear all other lines on PORTB
	cbi	DDRE, ADLC_D0		; clear ADLC_D0 line on PORTE

	clc
	
	in	r18, GICR			; save the contents of the General Interrupt Control Register
	ldi	tmp, (1 << INT1)	; Enable xx Interrupts
	out	GIFR, tmp			; write to the General Interrupt Flag Register
	out	GICR, tmp			; write to the General Interrupt Control Register


adlc_read_wait:				; wait for adlc_access to run
	brcc	adlc_read_wait	; r16 and r17 will return with PortB and PortE
	
	bst	r16, ADLC_D0		; Store the bit from ADLC_D0 in the T flag
	bld	r17, 0				; load T into bit 0 of r17 to leave the byte read in r17

	ldi	r16, 0xff			; set pins back to output
	out	DDRB, r16			; Port B
	sbi	DDRE, ADLC_D0		; ADLC_DO pin on PORT E
	ret


; -----------------------------------------------------------------------------------------
; ADLC Write
; -----------------------------------------------------------------------------------------
;
; r16=address
; r17=data

adlc_write:
	; set the addressing
	cbi	PORTD, ADLC_RS0		; clear RS0 on PORTD -
	cbi	PORTD, ADLC_RS1		; clear RS1 on PORTD - ADLC Rx
	sbrc	r16, 0			; if bit 0 in the address field is not clear
	sbi	PORTD, ADLC_RS0		;    set bit 0
	sbrc	r16, 1			; if bit 1 in the address field is not clear
	sbi	PORTD, ADLC_RS1		;    set bit 1
	cbi	PORTD, ADLC_RnW		; write to ADLC

	; set the data
	out	PORTB, r17			; put the data on PORT B
	cbi	PORTE, ADLC_D0		; clear bit 2 on PORT E
	sbrc	r17, 0			; if bit 0 in the data field is not clear
	sbi	PORTE, ADLC_D0		;    set bit to 1

	clc
	
	in	r18, GICR			; save the contents of the General Interrupt Control Register
	ldi	tmp, (1 << INT1)	; Enable xx Interrupts
	out	GIFR, tmp			; write to the General Interrupt Flag Register
	out	GICR, tmp			; write to the General Interrupt Control Register

adlc_write_wait:			; wait for adlc_access to run
	brcc adlc_write_wait	; by checking if carry is cleared
	ret

; -----------------------------------------------------------------------------------------
; ADLC access
; -----------------------------------------------------------------------------------------
;
; perform one read or write access cycle

adlc_access:
	nop
	nop
	cbi	PORTE, ADLC_nCE		; Clear the ADLC_nCE
	nop
	nop
	nop
	nop
	nop
	nop
	in	r17, PINB			; read PortB to r17
	in	r16, PINE			; read PortE to r16
	sbi	PORTE, ADLC_nCE		; set ADLC_nCE to flag read completed
	out	GICR, r18			; restore the contents of the Global Interrupt Control Register
	sec						; set carry to flag interrupt is handled
	reti					; return from interrupt

; -----------------------------------------------------------------------------------------
; ADLC Tx frame
; -----------------------------------------------------------------------------------------
;

adlc_tx_frame:
	ret

; -----------------------------------------------------------------------------------------
; ADLC Tx IRQ
; -----------------------------------------------------------------------------------------
;

adlc_tx_irq:
	rjmp	adlc_irq_ret

; -----------------------------------------------------------------------------------------
; ADLC IRQ
; -----------------------------------------------------------------------------------------
;

adlc_irq:	
	push	tmp
	push	r16
	push	r17
	in	r16, SREG
	push	r16

	; disable adlc irq and re-enable global interrupts
	clr	tmp
	out	GICR, tmp
	sei

	lds	tmp, adlc_state
	;bst	tmp, 7
	;brbs	6, adlc_tx_irq

	ldi	r16, ADLC_SR1	
	rcall	adlc_read

	bst	r17, 0		; RDA?
	brbs	6, process_rda		; yes
	bst	r17, 1		; S2RQ?
	brbs	6, process_s2rq		; yes

	ldi	r16, 0x56
	rcall	serial_tx

	rjmp	adlc_irq_ret

process_idle:
	ldi	r16, 0x69
	call	serial_tx

	clr	tmp
	sts	adlc_state, tmp

	rcall	adlc_clear_rx
	rjmp	adlc_irq_ret

process_s2rq:
	ldi	r16, ADLC_SR2
	rcall	adlc_read

	;bst	r17, 4		; DCD?
	;brbs	6, process_dcd		; yes
	bst	r17, 0		; AP?
	brbs	6, process_ap		; yes
	bst	r17, 1		; FV?
	brbs	6, process_fv		; yes
	bst	r17, 3		; Abort?
	brbs	6, abandon_rx		; yes
	bst	r17, 4		; CRC error?
	brbs	6, rx_error		; yes
	bst	r17, 6		; overrun?
	brbs	6, abandon_rx		; yes
	bst	r17, 2		; Idle?
	brbs	6, process_idle		; yes
	bst	r17, 7		; RDA?
	brbs	6, process_rda		; yes

	ldi	r16, 0x55
	rcall	serial_tx

	mov	r16, r17
	rcall	serial_tx_hex

process_ap:
	ldi	r16, 0x61
	call	serial_tx

	ldi	r16, ADLC_RX
	rcall	adlc_read

	lds	XL, adlc_rx_ptr
	lds	XH, adlc_rx_ptr + 1

	st	X+, r17

	sts	adlc_rx_ptr, XL
	sts	adlc_rx_ptr + 1, XH

	lds	r16, adlc_state
	inc	r16
	sts	adlc_state, r16

	; switch to 2-byte mode
	ldi	r16, ADLC_CR2
	ldi	r17, CR2val | CR2_TWOBYTE
	rcall	adlc_write	

	rjmp	process_s2rq

process_rda:
	lds	XL, adlc_rx_ptr
	lds	XH, adlc_rx_ptr + 1

	ldi	r16, ADLC_RX
	rcall	adlc_read
	st	X+, r17
	ldi	r16, ADLC_RX
	rcall	adlc_read
	st	X+, r17

	sts	adlc_rx_ptr, XL
	sts	adlc_rx_ptr + 1, XH

	rjmp	adlc_irq_ret

rx_error:
	rcall	adlc_rx_flush
abandon_rx:
	; abandon in-progress reception
	clr	tmp
	sts	adlc_state, tmp
	rcall	adlc_clear_rx
	rjmp	process_s2rq		; might be AP as well


; -----------------------------------------------------------------------------------------
; ADLC clear Rx
; -----------------------------------------------------------------------------------------
;
; clear rx status and switch back to 1-byte mode
;

adlc_clear_rx:
	ldi	r16, ADLC_CR2
	ldi	r17, CR2val | CR2_CLRRX
	rjmp	adlc_write

process_fv:
	ldi	r16, 0x76
	call	serial_tx

	rcall	adlc_rx_flush
	ldi	r16, FRAME_COMPLETE
	sts	adlc_state, r16

	ldi	r16, ADLC_CR1
	ldi	r17, CR1_TXRESET
	rcall	adlc_write

	rjmp	adlc_irq_ret

; -----------------------------------------------------------------------------------------
; ADLC Rx Flush
; -----------------------------------------------------------------------------------------
	
adlc_rx_flush:
	; switch to 1-byte mode, no PSE
	ldi	r16, ADLC_CR2
	clr	r17
	rcall	adlc_write

	; read SR2
	ldi	r16, ADLC_SR2
	rcall	adlc_read
	
	bst	r17, 7		; RDA?
	brbc	6, no_rda		; no

	ldi	r16, ADLC_RX
	rcall	adlc_read

	lds	XL, adlc_rx_ptr
	lds	XH, adlc_rx_ptr + 1
	st	X+, r17
	sts	adlc_rx_ptr, XL
	sts	adlc_rx_ptr + 1, XH
no_rda:
	rjmp	adlc_clear_rx

; -----------------------------------------------------------------------------------------
; ADLC IRQ ret
; -----------------------------------------------------------------------------------------

adlc_irq_ret:
	ldi	tmp, (1 << INT0)
	out	GICR, tmp
	pop	r16
	out	SREG, r16
	pop	r17
	pop	r16
	pop	tmp
	reti

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
	ldi	r17, CR1_AC | CR1_TXRESET | CR1_RXRESET	; Switch controlling access to CR3 & CR4
												; Resets all Tx, Rx status and FIFO registers
												; these will automatically go back to 0
	ldi	r16, ADLC_CR1							; set to ADLC Control Register 1
	rcall	adlc_write							; write to ADLC

	; set the initial values of Control Register 3
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

adlc_ready_to_receive:
 	clr	tmp
	sts	adlc_state, tmp

	ldi	XL, ECONET_RX_BUF & 0xff
	ldi	XH, ECONET_RX_BUF >> 8
	
	sts	adlc_rx_ptr, XL
	sts	adlc_rx_ptr + 1, XH
	
	ldi	r17, CR1_RIE | CR1_TXRESET
	ldi	r16, ADLC_CR1
	rcall	adlc_write

	ldi	r17, CR2val
	ldi	r16, ADLC_CR2
	rjmp	adlc_write
