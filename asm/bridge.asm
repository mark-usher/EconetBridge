	.include "m162def.inc"
	.device	ATmega162

.equ	ADLC_RS0	= $4
.equ	ADLC_RS1	= $0
.equ	ADLC_RnW	= $1
.equ	ADLC_D0		= $2
.equ	ADLC_nCE	= $0

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

.equ	ADLC_CR1		= 0
.equ	ADLC_CR2		= 1
.equ	ADLC_CR3		= 1
.equ	ADLC_TXCONTINUE		= 2
.equ	ADLC_TXTERMINATE	= 3
.equ	ADLC_CR4		= 3
.equ	ADLC_SR1		= 0
.equ	ADLC_SR2		= 1
.equ	ADLC_RX			= 2

.equ	CR1_AC			= 0x01
.equ	CR1_RIE			= 0x02
.equ	CR1_TIE			= 0x04
.equ	CR1_RDSR_MODE		= 0x08
.equ	CR1_TDSR_MODE		= 0x10
.equ	CR1_DISCONTINUE		= 0x20
.equ	CR1_RXRESET		= 0x40
.equ	CR1_TXRESET		= 0x80

.equ	CR2_PSE			= 0x01
.equ	CR2_TWOBYTE		= 0x02
.equ	CR2_FLAGIDLE		= 0x04
.equ	CR2_FC			= 0x08
.equ	CR2_TXLAST		= 0x10
.equ	CR2_CLRRX		= 0x20
.equ	CR2_CLRTX		= 0x40
.equ	CR2_RTS			= 0x80

.equ	CR3_LCF			= 0x01
.equ	CR3_CEX			= 0x02
.equ	CR3_AEX			= 0x04
.equ	CR3_ZERO_ONE		= 0x08
.equ	CR3_FDSE		= 0x10
.equ	CR3_LOOP		= 0x20
.equ	CR3_TST			= 0x40
.equ	CR3_DTR			= 0x80

.equ	CR4_DFSF		= 0x01
.equ	CR4_TXWLS		= 0x06
.equ	CR4_RXWLS		= 0x18
.equ	CR4_ABT			= 0x20
.equ	CR4_ABTEX		= 0x40
.equ	CR4_NRZI		= 0x80

.equ	SR1_RDA			= 0x01
.equ	SR1_S2RQ		= 0x02
.equ	SR1_LOOP		= 0x04
.equ	SR1_FD			= 0x08
.equ	SR1_CTS			= 0x10
.equ	SR1_TXU			= 0x20
.equ	SR1_TDRA		= 0x40
.equ	SR1_IRQ			= 0x80

.equ	SR2_AP			= 0x01
.equ	SR2_FV			= 0x02
.equ	SR2_RXIDLE		= 0x04
.equ	SR2_RXABT		= 0x08
.equ	SR2_ERR			= 0x10
.equ	SR2_DCD			= 0x20
.equ	SR2_OVRN		= 0x40
.equ	SR2_RDA			= 0x80

.equ	CR2val			= CR2_PSE ;| CR2_TWOBYTE

.equ	CS8900_BASE		= 0x8000
.equ	EGPIO_BASE		= 0xc000

.equ	EGPIO_ECONET_RED	= 0x0
.equ	EGPIO_ECONET_GREEN	= 0x1
.equ	EGPIO_STATUS_RED	= 0x2
.equ	EGPIO_ADLC_RESET	= 0x3
.equ	EGPIO_CLOCK_ENABLE	= 0x4
.equ	EGPIO_STATUS_GREEN	= 0x5
.equ	EGPIO_ETHER_RESET	= 0x6
.equ	EGPIO_TP1		= 0x7

.equ	EGPIO_SET		= 0x80

.equ	STACK_TOP		= 0x200

.equ	adlc_state		= 0x201
.equ	adlc_rx_ptr		= 0x202

.equ	ECONET_TX_BUF		= 0x6000
.equ	ECONET_RX_BUF		= 0x4000

.equ	RX_IDLE			= 0
.equ	RX_DATA			= 1
.equ	FRAME_COMPLETE		= 10

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
	ldi	r16, 0xff		; set ADLC data pins to output
	out	DDRB, r16
	sbi	DDRE, ADLC_D0

	ldi	r16, 0xf3		; interrupt pins as inputs
	out	DDRD, r16
	ldi	r16, 0xff		; all high
	out	PORTD, r16

	sbi	DDRE, ADLC_nCE
	sbi	PORTE, ADLC_nCE

	; set up timer 0 to generate ADLC clock waveform
	; select divide-by-10 for nominal 800kHz at 8MHz in
	ldi	r16, 4
	out	OCR0, r16
	; select CTC mode, toggle on OC, no prescaler
	ldi	r16, (1 << WGM01) | (1 << COM00) | (1 << CS00)
	out	TCCR0, r16

	; set up timer 1 to generate Econet clock output
	; select fast PWM with TOP=ICR1
	; clear OC on MATCH, set at TOP
	ldi	r16, (1 << WGM11) | (1 << COM1A1)
	out	TCCR1A, r16
	; no prescaling
	ldi	r16, (1 << WGM13) | (1 << WGM12) | (1 < CS10)
	out	TCCR1B, r16

	; set up external memory interface and interrupts
	; select 1 wait state for upper region at 0x8000-0xffff
	ldi 	r16, 1 << SRL2
	out	EMCUCR, r16
	ldi	r16, (1 << SRE) | (1 << ISC11) | (1 << SRW10)
	out	MCUCR, r16

	; initialise EGPIOs
	ldi	r16, EGPIO_ADLC_RESET
	rcall	egpio_write
	ldi	r16, EGPIO_STATUS_RED
	rcall	egpio_write
	ldi	r16, EGPIO_TP1 | EGPIO_SET
	rcall	egpio_write
	ldi	r16, EGPIO_ECONET_RED | EGPIO_SET
	rcall	egpio_write
	ldi	r16, EGPIO_ETHER_RESET | EGPIO_SET
	rcall	egpio_write
	ldi	r16, EGPIO_ECONET_GREEN | EGPIO_SET
	rcall	egpio_write
	ldi	r16, EGPIO_STATUS_GREEN | EGPIO_SET
	rcall	egpio_write
	ldi	r16, EGPIO_CLOCK_ENABLE | EGPIO_SET
	rcall	egpio_write

	; zero sram
	ldi	ZH, 0x5
	clr	tmp
	clr	ZL
zero1:
	st	Z, tmp
	inc	ZL
	brne	zero1
	inc	ZH
	brne	zero1

	rcall	crlf

	clr	r18
	out	GICR, r18

	sei

	; unreset ethernet chip
	ldi	r16, EGPIO_ETHER_RESET
	rcall	egpio_write

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

		; r16 [6:0] address [7] data
egpio_write:
	ldi	XH, EGPIO_BASE >> 8
	mov	XL, r16
	cbr	XL, EGPIO_SET
	bst	r16, 7
	clr	r16
	bld	r16, 0
	st	X, r16
	ret

; =======================================================================
; == Serial =============================================================
; =======================================================================
	; 115200bps bit time = 8.68us =~ 70 cycles @ 8MHz

	; r16 = byte to send
serial_tx:
	push	r18
	in	r18, SREG
	push	r18
	cli
	mov	r18, r16

	; send start bit
	ldi	r16, EGPIO_TP1
	rcall	egpio_write

	ldi	tmp, 8
serial_tx_loop:
	ldi	r16, 16
serial_tx_wait:
	dec	r16
	brne	serial_tx_wait
	ror	r18
	brcc	send_0
	ldi	r16, EGPIO_TP1 | EGPIO_SET
	rjmp	send_x
send_0:
	ldi	r16, EGPIO_TP1
	nop
	nop
send_x:
	rcall	egpio_write

	dec	tmp
	brne	serial_tx_loop

	ldi	r16, 18
serial_tx_wait2:
	dec	r16
	brne	serial_tx_wait2

	; send stop bit
	ldi	r16, EGPIO_TP1 | EGPIO_SET
	rcall	egpio_write

	ldi	r16, 100
serial_tx_wait3:
	dec	r16
	brne	serial_tx_wait3

	pop	r18
	out	SREG, r18
	pop	r18
	ret

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

crlf:
	ldi	r16, 13
	rcall	serial_tx
	ldi	r16, 10
	rjmp	serial_tx

; =======================================================================
; == ADLC ===============================================================
; =======================================================================
	
	; r16=address
	; r17=data
adlc_read:
	cbi	PORTD, ADLC_RS0
	cbi	PORTD, ADLC_RS1
	sbrc	r16, 0
	sbi	PORTD, ADLC_RS0
	sbrc	r16, 1
	sbi	PORTD, ADLC_RS1
	sbi	PORTD, ADLC_RnW

	ldi	r16, 0x1
	out	DDRB, r16
	cbi	DDRE, ADLC_D0

	clc
	
	in	r18, GICR
	ldi	tmp, (1 << INT1)
	out	GIFR, tmp
	out	GICR, tmp

adlc_read_wait:				;  wait for adlc_access to run
	brcc	adlc_read_wait
	
	bst	r16, ADLC_D0
	bld	r17, 0

	ldi	r16, 0xff		; set pins back to output
	out	DDRB, r16
	sbi	DDRE, ADLC_D0
	ret

	; r16=address
	; r17=data
adlc_write:
	cbi	PORTD, ADLC_RS0
	cbi	PORTD, ADLC_RS1
	sbrc	r16, 0
	sbi	PORTD, ADLC_RS0
	sbrc	r16, 1
	sbi	PORTD, ADLC_RS1
	cbi	PORTD, ADLC_RnW

	out	PORTB, r17
	cbi	PORTE, ADLC_D0
	sbrc	r17, 0
	sbi	PORTE, ADLC_D0

	clc
	
	in	r18, GICR
	ldi	tmp, (1 << INT1)
	out	GIFR, tmp
	out	GICR, tmp

adlc_write_wait:			;  wait for adlc_access to run
	brcc	adlc_write_wait
	ret

	; perform one read or write access cycle
adlc_access:
	nop
	nop
	cbi	PORTE, ADLC_nCE
	nop
	nop
	nop
	nop
	nop
	nop
	in	r17, PINB
	in	r16, PINE
	sbi	PORTE, ADLC_nCE
	out	GICR, r18
	sec
	reti	

adlc_tx_frame:
	ret

adlc_tx_irq:
	rjmp	adlc_irq_ret

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

	; clear rx status and switch back to 1-byte mode
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

adlc_irq_ret:
	ldi	tmp, (1 << INT0)
	out	GICR, tmp
	pop	r16
	out	SREG, r16
	pop	r17
	pop	r16
	pop	tmp
	reti

adlc_init:
	ldi	r16, EGPIO_ADLC_RESET | EGPIO_SET
	rcall	egpio_write		; release ADLC reset

	ldi	r17, CR1_AC | CR1_TXRESET | CR1_RXRESET
	ldi	r16, ADLC_CR1
	rcall	adlc_write

	clr	r17
	ldi	r16, ADLC_CR3
	rcall	adlc_write

	ldi	r17, CR4_TXWLS | CR4_RXWLS
	ldi	r16, ADLC_CR4
	rcall	adlc_write

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
	clr     chksumL		 ;we do the arithmetic
	clr     chksumM		 ; using a 24
	clr     chksumH		 ;  bit area
	sbrs    lengthL,0			;odd length?
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
