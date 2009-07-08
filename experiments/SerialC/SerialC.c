

#include <avr/io.h>
#include <avr/sleep.h>
#include "Serial.h"


extern void serial_tx(uint8_t mask);
extern void serial_tx_hex(uint8_t mask);
extern void egpio_init();


extern void aWriteIORegister();
extern void aReadIORegister();


void outportb( unsigned short reg, unsigned char val);
unsigned short inportb(unsigned short reg);
unsigned short ReadIORegister(unsigned short reg);
void WriteIORegister(unsigned short reg, unsigned short val);


int
main (void)
{
	CLKPR = 0x80;
	CLKPR = 0x0;

	// select 1 wait state for upper region at 0x8000-0xffff
	EMCUCR = (1 << SRL2);

	// set the MicroController Control Register
	MCUCR =  (1 << SRE) | (1 << ISC11) | (1 << SRW10); 
	/*		
	SRE	= 7		External SRAM Enable
	ISC111 		trigger on falling edge
	SRW10	= 6		External SRAM Wait State Select
	*/

	egpio_init();



  	unsigned short *ptr;
	unsigned char tmp;
	unsigned short i;
	
	// initial test read, gives back 630E
	i = ReadIORegister(CS8900_PDATA);
	serial_tx_hex((i & (unsigned short)0xFF00)>>8);
	serial_tx_hex((i & (unsigned short)0x00FF));
	
	// write out 0x01 0x02
	WriteIORegister(CS8900_PPTR, 0x0201);


	// should get 3 back
	i = ReadIORegister(CS8900_PDATA);
	serial_tx_hex((i & (unsigned short)0xFF00)>>8);
	serial_tx_hex((i & (unsigned short)0x00FF));


	/* ptr = CS8900_PDATA */
//	ptr = 0x0000;
	/* tmp = *ptr */
//	tmp = *ptr;
	// read the Packet Page Data Port. Should get 0x0E63
	/* output to serial */
//	serial_tx_hex(tmp);

/*
    for ( i=0; i<0xFFFF; i++)
     {
		tmp = *ptr++;
		if (tmp == 0x0E)
		{ 
		//  check next byte
		tmp = *ptr++;
		if (tmp == 0x63)
			{
				serial_tx_hex(tmp);
				serial_tx(0x20);
				serial_tx_hex(((i+(unsigned short)ptr) & (unsigned short)0xFF00)>>8);
				serial_tx_hex(((i+(unsigned short)ptr) & (unsigned short)0x00FF));
				serial_tx(10);
				serial_tx(13);
			}
		}
	}

		serial_tx(0x20);
*/


	/* loop forever */

	for (;;)
        sleep_mode();

	return (0);
}



/*******************************************************************/
/* ReadPPRegister(): Read value from the Packet Pointer register   */
/* at regOffset                                                    */
/*******************************************************************/
static unsigned short ReadPPRegister(unsigned short regOffset)
{
      // write a 16 bit register offset to IO port CS8900_PPTR
	outportb(CS8900_PPTR, (unsigned char)(regOffset & 0x00FF));
	outportb(((CS8900_PPTR + 1)), (unsigned char)((regOffset & 0xFF00) >> 8));

	// read 16 bits from IO port number CS8900_PPTR
      return (inportb(CS8900_PDATA) | (unsigned short)(inportb((CS8900_PDATA + 1)) << 8));
}

/*******************************************************************/
/* WritePPRegister(): write value to the Packet Pointer register   */
/* at regOffset                                                    */
/*******************************************************************/
void WritePPRegister(unsigned short regOffset, unsigned short val)
{
	// write a 16 bit register offset to IO port CS8900_PPTR
	outportb(CS8900_PPTR, (unsigned char)(regOffset & 0x00FF));
	outportb(((CS8900_PPTR + 1)), (unsigned char)((regOffset & 0xFF00) >> 8));

	// write 16 bits to IO port CS8900_PPTR
	outportb(CS8900_PDATA, (unsigned char)(val & 0x00FF));
	outportb(((CS8900_PDATA + 1)), (unsigned char)((val & 0xFF00) >> 8));
}

/*******************************************************************/
/* ReadPPRegisterHiLo(): Read value from the Packet Pointer        */
/* register at regOffset. this is a special case where we read the */
/* high order byte first then the low order byte.  This special    */
/* case is only used to read the RxStatus and RxLength registers   */
/*******************************************************************/
unsigned short ReadRxStatusLengthRegister()
{
	// read 16 bits from IO port number CS8900_PPTR
	return (inportb((CS8900_RTDATA + 1)) | (inportb((CS8900_RTDATA)) << 8));
}

/*******************************************************************/
/* WriteIORegister(): Write the 16 bit data in val to the register,*/
/* reg                                                             */
/*******************************************************************/
void WriteIORegister(unsigned short reg, unsigned short val)
{

	unsigned char temp1;
	unsigned char temp2;
	
	temp1 = (val & 0x00FF);
	temp2 = ((val & 0xFF00) >> 8);

	// Write the 16 bits of data in val to reg
	outportb(reg, (unsigned char)temp1);
	outportb((reg + 1), (unsigned char)temp2);

}


/*******************************************************************/
/* ReadIORegister(): Read 16 bits of data from the register, reg.  */
/*******************************************************************/
unsigned short ReadIORegister(unsigned short reg)
{

	unsigned char temp1;
	unsigned char temp2;

	temp1 = inportb(reg);
	temp2 = inportb(reg+1);

//	return(	inportb(reg) | inportb( ( (reg + 1) << 8)   ));	
	return( temp1 | (temp2 << 8)   );	

}



/* functions for the driver code to access memory */

void outportb( unsigned short reg, unsigned char val)
{


	// write the value in val to the register
	addr_lo=((unsigned char)(reg & 0x00FF));
	addr_hi=((unsigned char)((reg & 0xFF00) >> 8));
	data=val;

	// call write routine
	aWriteIORegister();
	return;

}

unsigned short inportb(unsigned short reg)
{
	addr_lo=((unsigned char)(reg & 0x00FF));
	addr_hi=((unsigned char)((reg & 0xFF00) >> 8));

	// call read routine
	aReadIORegister();

	return data;

}


