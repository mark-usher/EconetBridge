
#ifndef _CS8900_ASM_
#define _CS8900_ASM_


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


#endif  /* _CS8900_ASM_ */
