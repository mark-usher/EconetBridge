#define  CS8900_BASE (unsigned short) 0x8000 /* Default IO Port Address */

/* Directly visible registers through IP Port 
#define CS8900_RTDATA ((volatile unsigned char *) (CS8900_BASE+0x00))
#define CS8900_TxCMD  ((volatile unsigned char *) (CS8900_BASE+0x04))
#define CS8900_TxLEN  ((volatile unsigned char *) (CS8900_BASE+0x06))
#define CS8900_ISQ    ((volatile unsigned char *) (CS8900_BASE+0x08))
#define CS8900_PPTR   ((volatile unsigned char *) (CS8900_BASE+0x0a))
#define CS8900_PDATA  ((volatile unsigned char *) (CS8900_BASE+0x0c))
*/

#define CS8900_RTDATA ((unsigned short) (CS8900_BASE+0x00))
#define CS8900_TxCMD  ((unsigned short) (CS8900_BASE+0x04))
#define CS8900_TxLEN  ((unsigned short) (CS8900_BASE+0x06))
#define CS8900_ISQ    ((unsigned short) (CS8900_BASE+0x08))
#define CS8900_PPTR   ((unsigned short) (CS8900_BASE+0x0A))
#define CS8900_PDATA  ((unsigned short) (CS8900_BASE+0x0C))


/*
 * Global register variables.
 */
#ifdef __ASSEMBLER__

#  define addr_lo		r3
#  define addr_hi		r4
#  define data		r2

#else  /* !ASSEMBLER */

#include <stdint.h>

register uint8_t addr_lo	asm("r3");
register uint8_t addr_hi	asm("r4");
register uint8_t data		asm("r2");

#endif /* ASSEMBLER */
