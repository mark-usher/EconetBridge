; test to include C calls from Assembler
.include "m162def.inc"

; =======================================================================
; == Memory Map SRAM ====================================================
; =======================================================================

.equ	STACK_TOP		= 0x200


; =======================================================================
; == mega162 Vector table ===============================================
; =======================================================================

;.org $0000
rjmp	reset						; Reset vector (when the MCU is reset)
nop
nop


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

NAME get_port

#include "io8515.h" 		; The #include file must be within the module
EXTERN rand, max_val 		; External symbols used in the function
PUBLIC get_port 			; Symbols to be exported to C function
RSEG CODE 				; This code is relocatable, RSEG

get_port: ; Label, start execution here
	clr R16 ; Clear R16
	sbis PIND,0 ; Test if PIND0 is 0
	rcall rand ; Call RAND() if PIND0 = 0
	out PORTB,R16 ; Output random value to PORTB
	lds R17,max_val ; Load the global variable max_val
	cp R17,R16 ; Check if number higher than max_val
	brlt nostore ; Skip if not
	sts max_val,R16 ; Store the new number if it is higher
nostore:
	ret 	; Return

END
