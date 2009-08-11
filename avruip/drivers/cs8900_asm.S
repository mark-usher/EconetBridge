#define CS8900_BASE 0x8000 /* Default IO Port Address */

#define CS8900_RTDATA ((CS8900_BASE+0x00))
#define CS8900_TxCMD  ((CS8900_BASE+0x04))
#define CS8900_TxLEN  ((CS8900_BASE+0x06))
#define CS8900_ISQ    ((CS8900_BASE+0x08))
#define CS8900_PPTR   ((CS8900_BASE+0x0A))
#define CS8900_PDATA  ((CS8900_BASE+0x0C))

	;; r24/r25 buffer
	;; r22/r23 length
	.globl	cs8900SendPacketData
cs8900SendPacketData:
	mov	r31, r25
	mov	r30, r24
	mov	r18, r22
	andi	r22, 0xfe
	clr	r24
	clr	r25
	sbc	r24, r22
	sbc	r25, r23
	breq	done
loop:		
	ld	r20, Z+
	ld	r21, Z+
	sts	CS8900_RTDATA, r20
	sts	CS8900_RTDATA + 1, r21
	adiw	r24, 2
	brcc	loop
done:	
	sbrs	r18, 0
	ret
	ld	r20, Z
	sts	CS8900_RTDATA, r20
	ret

	.globl	cs8900RetreivePacketData
cs8900RetreivePacketData:
	mov	r31, r25
	mov	r30, r24
	mov	r18, r22
	andi	r22, 0xfe
	clr	r24
	clr	r25
	sbc	r24, r22
	sbc	r25, r23
	breq	2f
1:	
	lds	r20, CS8900_RTDATA
	lds	r21, CS8900_RTDATA + 1
	st	Z+, r20
	st	Z+, r21
	adiw	r24, 2
	brcc	1b	
2:	
	sbrs	r18, 0
	ret
	lds	r20, CS8900_RTDATA
	st	Z, r20
	ret

	.globl	ReadPPRegister
ReadPPRegister:
	sts	CS8900_PPTR, r24
	sts	CS8900_PPTR+1, r25
	lds	r24, CS8900_PDATA
	lds	r25, CS8900_PDATA+1
	ret

	.globl	WritePPRegister
WritePPRegister:	
	sts	CS8900_PPTR, r24
	sts	CS8900_PPTR+1, r25
	sts	CS8900_PDATA, r22
	sts	CS8900_PDATA+1, r23
	ret
	

#define PP_IA        0x0158  /* Individual address (MAC)*/

	.global cs8900LoadMac
cs8900LoadMac:
	mov	r31, r25
	mov	r30, r24
	ldi	r24, lo8(PP_IA)
	ldi	r25, hi8(PP_IA)
	ld	r22, Z+
	ld	r23, Z+
	rcall	WritePPRegister
	ld	r22, Z+
	ld	r23, Z+
	adiw	r24, 2
	rcall	WritePPRegister
	ld	r22, Z+
	ld	r23, Z+
	adiw	r24, 2
	rjmp	WritePPRegister
	
	
