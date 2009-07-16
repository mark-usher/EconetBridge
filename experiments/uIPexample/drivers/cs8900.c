/*===================================================================

File Name: 8bit8900.c

Software Utility program for CS8900A Ethernet chip to demostrate Tx
and Rx in 8-bit mode.

=====================================================================
History:

  3/28/02 Tanya   -Created the program based on the file CS8900.c

===================================================================*/

//#include <stdio.h>
//#include <dos.h>
//#include <conio.h>
#include "cs8900.h"
#include "delay.h"


extern void serial_tx(uint8_t mask);
extern void serial_tx_hex(uint8_t mask);
extern void egpio_init(void);

extern void aWriteIORegister(void);
extern void aReadIORegister(void);

//
// Function prototypes
//
/*
// unsigned short ReadPPRegister(unsigned short regOffset);
*/
void cs8900Init(void);
void cs8900BeginPacketSend(unsigned int);
void cs8900SendPacketData(unsigned char * , unsigned int);
void cs8900EndPacketSend(void);
unsigned int cs8900BeginPacketRetreive(void);
void cs8900RetreivePacketData(unsigned char * , unsigned int );
void cs8900EndPacketRetreive(void);

static unsigned short ReadPPRegister(unsigned short );
void WritePPRegister(unsigned short, unsigned short);
unsigned short ReadRxStatusLengthRegister(void);
void WriteIORegister(unsigned short , unsigned short );
unsigned short ReadIORegister(unsigned short );
int cs8900_poll_init(unsigned short);
int cs8900_reset(void);
void outportb( unsigned short, unsigned char);
unsigned short inportb(unsigned short);

void serial_short(unsigned short);
void serial_ok(unsigned char);
void serial_rx(void);
void serial_error(unsigned char);
void serial_crlf(void);


unsigned char gEtherAddr[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
int gPrevTxBidFail = 0;
int gTxInProgress = 0; 


//
//	gRxDataBuf - Rx data buffer
//	gTxDataBuf - Tx data buffer
//	gTxLength - Tx frame is always 1513 bytes in length
//
extern unsigned char gRxDataBuf[];
extern unsigned char gTxDataBuf[];
extern int gTxLength;

/* Tx and Rx statistics for Interrupt Mode*/
extern int  gIntTxErr , gIntTxOK , gIntRxErr , gIntRxOK, gBufEvent_Rdy4Tx;

extern unsigned int gIrqNo;/* The CS8900's IRQ number.  It can be 5, 10, 11, and 12. */

/* wrappers for the driver */

void cs8900Init(void)
{ 
	int status;
	status = cs8900_poll_init(PP_TestCTL_FDX);
	return;
}

void cs8900BeginPacketSend(unsigned int packetLength)
{ 

     int total_len = packetLength;
     unsigned short stat;

	//****** Step 0: First thing to do is to disable all interrupts
	//               at processor level.
	//
	// The following steps must be atomic: Write the TX command,
	// Bid for Tx, and copy Tx data to chip.  If they are not atomic,
	// the CS8900 may hang if an interrupt occurs in the middle of
	// these steps.
	// Since the program runs in real DOS mode and CS8900's interrupt
	// is not enabled, we don't need the step 0 here.
	//
	// disable();

	//****** Step 1:  Write the TX command
	//
	if (gPrevTxBidFail)
	{
		// Previous BidForTX has reserved the Tx FIFO on CS8900, The
		// FIFO must be released before proceeding. Setting the
		// PP_TxCmd_Force bit will cause CS8900 to release the
		// previously reserved Tx buffer.
		//
		WriteIORegister(CS8900_TxCMD,
			(unsigned short) PP_TxCmd_TxStart_Full | PP_TxCmd_Force);
		gPrevTxBidFail=0;
	}
	else
	{
		WriteIORegister(CS8900_TxCMD,
		(unsigned short) PP_TxCmd_TxStart_Full);
	}

	//***** Step 2: Bid for Tx
	//
	// Write the frame length  (number of bytes to TX).
	// Note: After the frame length has been written, the CS8900
	// reserves the Tx buffer for this bid whether PP_BusStat_TxRDY
	// is set or not.
	//
	WriteIORegister(CS8900_TxLEN, (unsigned short) total_len);

	// Read BusST to verify it is set as Rdy4Tx.
	stat = ReadPPRegister(PP_BusStat);

	//***** Step 3: Check for a Tx Bid Error (TxBidErr bit)
	//
	if (stat & PP_BusStat_TxBid)
	{
	// TxBidErr happens only if Tx_length is too small or
	// too large.
	//printf("cs8900_poll_send(): Tx Bid Error! TxLEN=%d \n", total_len);

	//***** Step 3.1: enable interrupts at processor level if it
	//                is disabled in step 0.
	//
	// enable( );
	return;
	}


	//***** Step 4: check if chip is ready for Tx now
	//
	// If Bid4Tx not ready, skip the frame
	//
	if ( (stat & PP_BusStat_TxRDY) == 0)
	{
		// If not ready for Tx now, abort this frame.
		// Note: Another alternative is to poll PP_BusStat_TxRDY
		// until it is set, if you don't want to abort Tx frames.

		// Set to 1, and next time cs8900_poll_send() is called, it
		// can set PP_TxCmd_Force to release the reserved Tx buffer
		// in the CS8900 chip.
		gPrevTxBidFail=1;
		//printf("cs8900_poll_send(): Tx Bid Not Ready4Tx! TxLEN=%d\n",total_len);

		//***** Step 4.1: enable interrupts at processor level if it
		//                is disabled in step 0.
		//
		// enable( );
		return;
	}

	return;
}

void cs8900SendPacketData(unsigned char * localBuffer, unsigned int length)
{
	int len;
	int total_len = length;
	unsigned short *sdata;
	unsigned short stat;
	unsigned long i;
 
	//***** Step 5: copy Tx data into CS8900's buffer
	//
	// This actually starts the Txmit
	//
	sdata = localBuffer;
	len = total_len;
	if (len > 0)
	{
		// Output contiguous words, two bytes at a time.
		while (len > 1)
		{
			WriteIORegister(CS8900_RTDATA, *sdata);
			sdata++;
			len -= 2;
		}

		// If Odd bytes, copy the last one byte to chip.
		if (len == 1)
		{
			outportb(CS8900_RTDATA, (*sdata) & 0x00ff);
		}
	}

	//***** Step 6: Poll the TxEvent Reg for the TX completed status
	//
	// This step is optional. If you don't wait until Tx compelete,
	// the next time cs8900_poll_send() bids for Tx, it may encounter
	// Not Ready For Tx because CS8900 is still Tx'ing.
	for ( i=0; i<30000; i++)
	{
		stat = ReadPPRegister(PP_TER);
		if ( stat != 0x0008 )
		{
		    break;
		}
	}

     // Tx compelete without error, return total_length Tx'ed
     if ( (stat & PP_TER_TxOK) || (stat == 0x0008) )
     {
	return;// total_len; /* return successful*/
     }

     // Tx with Errors
     //printf("cs8900_poll_send(): Tx Error! TxEvent=%x \n", stat);
     // Tx Failed
     return; // -1;

}

void cs8900EndPacketSend(void)
{ 
	return;
}

unsigned int cs8900BeginPacketRetreive(void)
{ 
     unsigned short stat, totalLen, val;

     //***** Step 1: check RxEvent Register
     //
     stat = ReadPPRegister(PP_RER);

     //***** Step 2: Determine if there is Rx event.
     //
     // If nothing happened, then return. 0x0004 is the register ID
     // If some bits between Bit 6 - Bit 15 are set, an Rx event
     // happened.
     //
	if ( stat == 0x0004 )
	{
		return 0;
	}

	//***** Step 3: Determine if there is Rx Error.
	//
	// If RxOk bit is not set, Rx Error occurred
	//
	if ( !(stat & PP_RER_RxOK) )
	{
		if (  stat & PP_RER_CRC)
		{
			//printf("cs8900_poll_recv(): Rx frame with CRC error\n");
		}
	else if ( stat & PP_RER_RUNT)
		{
			//printf("cs8900_poll_recv(): Rx frame with RUNT error\n");
		}
	else  if (stat & PP_RER_EXTRA )
		{
	    		//printf("cs8900_poll_recv(): Rx frame with EXTRA error\n");
		}
	else
		//printf("cs8900_poll_recv(): Unknown Error: stat=%x\n", stat);

		//***** Step 4: skip this received error frame.
		//
		// Note: Must skip this received error frame. Otherwise,
		// CS8900 hangs here.
		//
		// Read the length of Rx frame
		ReadRxStatusLengthRegister();
		ReadRxStatusLengthRegister();

		// Write Skip1 to RxCfg Register and also keep the current
		// configuration.
		val = ReadPPRegister(PP_RxCFG);
		val |= PP_RxCFG_Skip1;
		WritePPRegister(PP_RxCFG, val);
		return 0; /* return failed */
     }

	//***** Step 5: Read the Rx Status, and Rx Length Registers.
	totalLen = ReadRxStatusLengthRegister();
	totalLen = ReadRxStatusLengthRegister();

	// uncomment printf for debug
	// printf("RxEvent - stat: %x, len: %d\n", stat, totalLen);


	return (unsigned int)totalLen;
}

void cs8900RetreivePacketData(unsigned char * localBuffer, unsigned int length)
{ 

	unsigned short totalLen, val;
	int leftLen;
	unsigned short *data;
	unsigned char *cp;

	totalLen=(unsigned int)length;

	//***** Step 6: Read the Rx data from Chip and store it to
	//              user buffer.
	//
	data = (unsigned short *)localBuffer;
	leftLen = totalLen;


	// Read 2 bytes at a time
	while (leftLen >= 2)
	{
	*data++ = ReadIORegister(CS8900_RTDATA);
	leftLen -= 2;
	}

     // If odd bytes, read the last byte from chip
     if (leftLen == 1)
     {
	// Read the last byte from chip
	val = inportb(CS8900_RTDATA);

	// Point to the last one byte of the user buffer
	cp = (unsigned char *)data;

	// Truncate the word (2-bytes) read from chip to one byte.
	*cp = (unsigned char)(val & 0xff);
     }

	return;
}

void cs8900EndPacketRetreive(void)
{ 
	return;
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


	unsigned char temp1;
	unsigned char temp2;
	
	temp1 = (regOffset & 0x00FF);
	temp2 = ((regOffset & 0xFF00) >> 8);

	// write a 16 bit register offset to IO port CS8900_PPTR
	outportb(CS8900_PPTR, temp1);
	outportb(((CS8900_PPTR + 1)), temp2);

	// write 16 bits to IO port CS8900_PPTR
//	outportb(CS8900_PDATA, (unsigned char)(val & 0x00FF));
//	outportb(((CS8900_PDATA + 1)), (unsigned char)((val & 0xFF00) >> 8));

	temp1 = (val & 0x00FF);
	temp2 = ((val & 0xFF00) >> 8);

	// Write the 16 bits of data in val to reg
	outportb(CS8900_PDATA, (unsigned char)temp1);
	outportb((CS8900_PDATA + 1), (unsigned char)temp2);

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
      return ((unsigned short)(inportb((CS8900_RTDATA + 1)) << 8) | inportb(CS8900_RTDATA));

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


	// Write the 16 bits of data in val to reg
//	outportb((unsigned short *)reg, (unsigned char)(val & 0x00FF));
//	outportb(((unsigned short *)(reg + 1)), (unsigned char)((val & 0xFF00) >> 8));

}

/*******************************************************************/
/* ReadIORegister(): Read 16 bits of data from the register, reg.  */
/*******************************************************************/
unsigned short ReadIORegister(unsigned short reg)
{
	return(inportb((unsigned short *)reg) | (inportb((unsigned short *)(reg + 1)) << 8));
//	return((inportb((unsigned short *)reg)) <<8 | (inportb((unsigned short *)(reg + 1)) ));
}


/*******************************************************************/
/*   cs8900_poll_init( ): start up CS8900 for Poll Mode            */
/*******************************************************************/
int cs8900_poll_init(unsigned short duplexMode)
{
     unsigned short chip_type, chip_rev;
     unsigned short tmpAddr0, tmpAddr1, tmpAddr2;
	  unsigned short *ptr;
	  int status;

	unsigned char tmp;


     // Information: read Chip Id and Revision
     chip_type = ReadPPRegister(PP_ChipID);
     chip_rev = ReadPPRegister(PP_ChipRev);

	serial_crlf();

     //printf("CS8900 - type: %x, rev: %x\n", chip_type, chip_rev);
	serial_tx(0x54);
	serial_short(chip_type);

	serial_tx(0x52);
	serial_short(chip_rev);
	serial_crlf();

    /****** step 1: software reset the chip ******/
    status = cs8900_reset();
    if (status != 0)
    {
      //printf("CS8900 Error: Reset Failed!\n");
	return -1;
    }

    //***** step 2: Set up Ethernet hardware address
    //
    // Note: Due to the strange problem in Borland C/C++ 4.52, cannot
    // use a for loop.  If WritePPRegister(PP_IA+i*2, *(ptr+i)) is
    // used, the value of *(ptr+i) is always 0.
    //
    ptr = (unsigned short *)gEtherAddr;
    tmpAddr0=*ptr;
    tmpAddr1=*(ptr+1);
    tmpAddr2=*(ptr+2);

    // Write 2 bytes of Ethernet address into the Individual Address
    // register at a time
    WritePPRegister(PP_IA, tmpAddr0 );
    WritePPRegister(PP_IA+2, tmpAddr1 );
    WritePPRegister(PP_IA+4, tmpAddr2 );

    //***** step 3: Configure RxCTL to receive good frames for
    //              Indivual Addr, Broadcast, and Multicast.
    //
    // WritePPRegister(PP_RxCTL, PP_RxCTL_RxOK | PP_RxCTL_IA |
    //                  PP_RxCTL_Broadcast | PP_RxCTL_Multicast);

    //***** step 3: or set to Promiscuous mode to receive all
    //              network traffic.
    //
    WritePPRegister(PP_RxCTL, PP_RxCTL_Promiscuous|PP_RxCTL_RxOK);


    //***** step 4: Configure TestCTL (DuplexMode)
    //

    // defualt: 0:half duplex;  0x4000=Full duplex
    WritePPRegister(PP_TestCTL, duplexMode);


    //***** step 5: Set SerRxOn, SerTxOn in LineCTL
    //
    WritePPRegister(PP_LineCTL, PP_LineCTL_Rx | PP_LineCTL_Tx);
	    
    return 0;

}

/*******************************************************************/
/*   cs8900_reset( ): software reset the CS8900 chip               */
/*******************************************************************/
int cs8900_reset(void)
{
     int j;
     unsigned short status;

     // Reset chip
     WritePPRegister(PP_SelfCtl, (unsigned short)PP_SelfCtl_Reset);

     // Wait about 125 msec for chip resetting in progress
     for (j=0; j<100; j++)
     {
	delay_ms(125);
     }

     // check the PP_SelfStat_InitD bit to find out if the chip
     // successflly reset
     for (j=0; j<3000; j++)
     {
	status=(ReadPPRegister(PP_SelfStat) & PP_SelfStat_InitD);
	if ( status != 0 )
	{
	    // Successful
	    return 0;
	}
     }

     // Failure
     return -1;
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




void serial_short(unsigned short val)
{

	unsigned short tmp;


	tmp = ((val & 0xFF00) >>8);		// calc high byte
	serial_tx_hex((unsigned char)tmp);	// output high byte in hex

	serial_tx(0x20);				// space
	
	tmp = (val & 0x00FF);			// calc lo byte
	serial_tx_hex((unsigned char)tmp);	// output high byte in hex

	return;

}


void serial_ok(unsigned char okVal)
{

	serial_tx(0x6f);		// o
	serial_tx(0x6b);		// k
	serial_tx(0x20);		// space
	serial_tx_hex(okVal);	// value
	serial_crlf(); 		// cr lf

	return;
}

void serial_rx(void)
{

	unsigned short tmp;

	serial_tx(0x72);		// r
	serial_tx(0x78);		// x
	serial_tx(0x20);		// space
	return;
}

void serial_error(unsigned char eVal)
{

	serial_tx(0x65);		// e
	serial_tx(0x20);		// space
	serial_tx_hex(eVal);	// value
	serial_crlf(); 		// cr lf

	return;
}
void serial_crlf(void)
{

	serial_tx(13);		// cr
	serial_tx(10);		// lf

	return;
}
