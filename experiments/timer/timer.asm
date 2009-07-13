.include "m162def.inc"


.def	tmp			= r24


; =======================================================================
; == External GPIO ======================================================
; =======================================================================

.equ	EGPIO_ADLC_RESET	= 0x3			; ADLC Reset
.equ	EGPIO_ETHER_RESET	= 0x6			; Ethernet Reset
.equ	EGPIO_TP1		= 0x7			; Serial TX Output
.equ	EGPIO_CLOCK_ENABLE= 0x4			; Econet Clock Enable

.equ	EGPIO_SET		= 0x80		; flag to set


; =======================================================================
; == Memory Map SRAM ====================================================
; =======================================================================

.equ	STACK_TOP		= 0x200
.equ	EGPIO_BASE		= 0xc000


; =======================================================================
; == Set the initial vectors ============================================
; =======================================================================

.org $0000
rjmp Reset ; Reset vector

.org OC3Aaddr
rjmp Timer3 ; T/C3 3A address

; Reset vector - just sets up interrupts and service routines and
; then loops forever.
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


	; set up timer 3 to generate timer clock output
	ldi	r16, ((1 << WGM32) | 1 << COM3A1)
	sts	TCCR3A, r16			; TCCR3A - Timer/Counter3 Control Register A


	; to get ticks at 1Hz use a prescaler of f CLK_I/O /256 (CS3x bits in TCCR3B)
	; this gives input frequency of 31250Hz from an 8 MHz clock
;	ldi	r16, (1 << CS30)		; No prescaler
	ldi	r16, (1 << CS32)		; prescaler / 256 
	sts	TCCR3B, r16			; TCCR3B - Timer/Counter3 Control Register B

	; timer3 overflow interrupt enable
	ldi	r16, (1 << OCIE3A ) 	; TOIE3
	sts ETIMSK, r16

	; set OCR3A to 31248 0x7A10

	ldi r16, 0x7A
	sts OCR3AH, r16
	ldi r16, 0x10
	sts OCR3AL, r16

	; generates timer3 compare match A interrupts through vector OC3Aaddr	
	; Timer/Counter3 Compare Match A
	; when the timer rolls over
	
	;increment counter

; =======================================================================
; == setup IO ===========================================================
; =======================================================================


	; set up external memory interface 
	; select 1 wait state for upper region at 0x8000-0xffff
	ldi r16, 1 << SRL2			; SRL2	= 6		Wait State Sector Limit Bit 2
	out	EMCUCR, r16				; set the Extended MCU Control Registe

	ldi	r16, (1 << SRE) | (1 << SRW10)		
	out	MCUCR, r16				; set the MicroController Control Register

	rcall	egpio_init

	sei						; set interrupts


loop:
	rjmp loop



Timer3:
	cli
	push r16
	
	clr r16
	
	sts TCNT3H, r16
	sts TCNT3L, r16
	sts ETIFR, r16

	ldi r16, up_B
	rcall serial_tx
	
	pop r16
	sei
	
	reti

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
	ldi	r16, EGPIO_TP1 | EGPIO_SET 	; 7
	rcall	egpio_write
	ldi	r16, EGPIO_CLOCK_ENABLE | EGPIO_SET	
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

.include "serial.asm"


