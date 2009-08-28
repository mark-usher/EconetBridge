/*===================================================================

File Name: 8bit8900.c

Software Utility program for CS8900A Ethernet chip to demostrate Tx
and Rx in 8-bit mode.

=====================================================================
History:

  3/28/02 Tanya   -Created the program based on the file CS8900.c

===================================================================*/

#include "cs8900.h"
#include "delay.h"
#include "uip.h"
#include "globals.h"

static unsigned char gPrevTxBidFail;

extern unsigned short ReadPPRegister(unsigned short);
static void WritePPRegister(unsigned short, unsigned short) __attribute__ (( noinline ));
static unsigned short ReadRxStatusLengthRegister(void);
static void WriteIORegister(unsigned short , unsigned short );
static int8_t cs8900_poll_init(unsigned short);
static int8_t cs8900_reset(void);


/* functions for the driver code to access memory */
static void outportb( unsigned short reg, unsigned char val)
{
	*((volatile unsigned char *)reg) = val;
}

static unsigned short inportb(unsigned short reg)
{
	return *((volatile unsigned char *)reg);
}

void cs8900Init(void)
{
	cs8900_poll_init(PP_TestCTL_FDX);
}

void cs8900BeginPacketSend(uint16_t packetLength)
{

     uint16_t total_len = packetLength;
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

void cs8900EndPacketSend(void)
{
	unsigned short stat;
	uint16_t i;

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

}

uint16_t cs8900BeginPacketRetreive(void)
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

	return totalLen;
}

/*******************************************************************/
/* WritePPRegister(): write value to the Packet Pointer register   */
/* at regOffset                                                    */
/*******************************************************************/
static void WritePPRegister(unsigned short regOffset, unsigned short val)
{

	// write a 16 bit register offset to IO port CS8900_PPTR
      outportb(CS8900_PPTR, (unsigned char)(regOffset & 0x00FF));
	outportb((CS8900_PPTR + 1),
	(unsigned char)((regOffset & 0xFF00) >> 8));

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
static unsigned short ReadRxStatusLengthRegister()
{

	// read 16 bits from IO port number CS8900_PPTR
      return ((unsigned short)(inportb((CS8900_RTDATA + 1)) << 8) | inportb(CS8900_RTDATA));

}

/*******************************************************************/
/* WriteIORegister(): Write the 16 bit data in val to the register,*/
/* reg                                                             */
/*******************************************************************/
static void WriteIORegister(unsigned short reg, unsigned short val)
{

	// Write the 16 bits of data in val to reg
	outportb((unsigned short *)reg, (unsigned char)(val & 0x00FF));
	outportb(((unsigned short *)(reg + 1)), (unsigned char)((val & 0xFF00) >> 8));

}

/*******************************************************************/
/*   cs8900_poll_init( ): start up CS8900 for Poll Mode            */
/*******************************************************************/
static int8_t cs8900_poll_init(unsigned short duplexMode)
{
	unsigned short chip_type, chip_rev;
 	unsigned short tmpAddr0, tmpAddr1, tmpAddr2;
	unsigned short *ptr;
	int8_t status;

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
    ptr = (unsigned short *)&eeprom.MAC[0];
    cs8900LoadMac(ptr);

    //***** step 3: Configure RxCTL to receive good frames for
    //              Indivual Addr, Broadcast, and Multicast.
    //
    WritePPRegister(PP_RxCTL, PP_RxCTL_RxOK | PP_RxCTL_IA |
		    PP_RxCTL_Broadcast | PP_RxCTL_Multicast);


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
static int8_t cs8900_reset(void)
{
     unsigned short j;
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



