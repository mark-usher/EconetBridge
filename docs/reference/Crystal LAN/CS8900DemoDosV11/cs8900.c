/*==========================================================================

File Name: CS8900.c

Software Utility program for CS8900A Ethernet chip to demostrate Tx and Rx in Poll 
and Interrupt Modes.

==========================================================================
History:

  04/16/02 Melody   -Fixed the bug that forget to enable the interrupt for ReadyForTx

  10/26/01 Melody   -Created the program.

==========================================================================
*/

#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include "cs8900.h"

int cs8900_reset(void);
unsigned short ReadPPRegister(unsigned short regOffset);
void WritePPRegister(unsigned short regOffset, unsigned short val);
void cs8900_EnableInt(void);
void cs8900_DisableInt(void);
void CopyTxFrameToChip(unsigned char *dataBuf, int total_len);
int ProcessTxStatus(unsigned short TxEventStatus);

unsigned char gEtherAddr[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};/* Ethernet physical address of the chip*/
int gPrevTxBidFail = 0; /* The flag to know if the previous bid for Tx is failed. */
int gTxInProgress = 0; /* The flag to know if Tx is in progress (not compelete) */

void interrupt (*gOldIntHandlerPtr)(); /* pointer to the old Interrupt function*/

/* The followings are used for IBM compitable PC to hook up interrupt service routine */
unsigned int PICirr;
unsigned int PICimr;
unsigned int PICint;
unsigned int PICmask;


extern unsigned char gRxDataBuf[];/* buffer to hold Rx data */
extern unsigned char gTxDataBuf[];/* buffer to hold Tx data */
extern int gTxLength; /* Always Tx 1513-byte-long frame */
extern int  gIntTxErr , gIntTxOK , gIntRxErr , gIntRxOK; /* Tx and Rx statistics for Interrupt Mode*/
extern unsigned int gIrqNo;/* The CS8900's IRQ number.  It can be 5, 10, 11, and 12. */

/***************************************************************************************/
/* ReadPPRegister(): Read value from the Packet Pointer register at regOffset          */
/***************************************************************************************/
static unsigned short ReadPPRegister(unsigned short regOffset)
{
	 /* write a word (16 bits) regOffset to IO port number CS8900_PPTR */
	 outport(CS8900_PPTR, regOffset);

	 /* read a word (16 bits) from IO port number CS8900_PPTR */
	return (inport(CS8900_PDATA));
}

/****************************************************************************************/
/* WritePPRegister(): write value to the Packet Pointer register at regOffset           */
/****************************************************************************************/
void WritePPRegister(unsigned short regOffset, unsigned short val)
{
	outport(CS8900_PPTR, regOffset);
	outport(CS8900_PDATA, val); 
}


/**************************************************************************/
/*   cs8900_poll_init( ): start up CS8900 for Poll Mode                   */
/**************************************************************************/
int cs8900_poll_init(unsigned short duplexMode) {

	unsigned short chip_type, chip_rev, chip_status, tmpAddr0, tmpAddr1, tmpAddr2;
    unsigned short *ptr;
	int status;

	/* Information: read Chip Id and Revision */
 	chip_type = ReadPPRegister(PP_ChipID);
    chip_rev = ReadPPRegister(PP_ChipRev);
	printf("CS8900 - type: %x, rev: %x\n", chip_type, chip_rev);

    /****** step 1: software reset the chip ******/
	status = cs8900_reset();
	if (status != 0 ) {
      printf("CS8900 Error: Reset Failed!\n");
      return -1;
	}

    /* Information: Is EEPROM present? */
	chip_status = ReadPPRegister(PP_SelfStat);
    printf("CS8900 - status: %x (%sEEPROM present)\n", chip_status, chip_status&PP_SelfStat_EEPROM ? "" : "no ");

 
	/****** step 2: Set up Ethernet hardware address ******/
    /* Note: Due to the wiered problem in Borland C/C++ 4.52, cannot use a for loop.  If 
		     WritePPRegister(PP_IA+i*2, *(ptr+i)) is used, the value of *(ptr+i) is always 0.*/
	ptr = (unsigned short *)gEtherAddr;
	tmpAddr0=*ptr;
 	tmpAddr1=*(ptr+1);
 	tmpAddr2=*(ptr+2);
    /* Write 2 bytes of Ethernet address into the Individual Address register at a time */
    WritePPRegister(PP_IA, tmpAddr0 );
    WritePPRegister(PP_IA+2, tmpAddr1 );
    WritePPRegister(PP_IA+4, tmpAddr2 );

 

    /****** step 3: Configure RxCTL to receive good frames for Indivual Addr, Broadcast, and Multicast.*/  
    /*WritePPRegister(PP_RxCTL, PP_RxCTL_RxOK | PP_RxCTL_IA | PP_RxCTL_Broadcast | PP_RxCTL_Multicast);*/
    
    /****** step 3: or set to Promiscuous mode to receive all network traffic ******/
	 WritePPRegister(PP_RxCTL, PP_RxCTL_Promiscuous|PP_RxCTL_RxOK); 
    

    /****** step 4: Configure TestCTL (DuplexMode) ******/
	/* defualt: 0:half duplex;  0x4000=Full duplex*/
    WritePPRegister(PP_TestCTL, duplexMode);


    /****** step 5: Set SerRxOn, SerTxOn in LineCTL ******/
    WritePPRegister(PP_LineCTL, PP_LineCTL_Rx | PP_LineCTL_Tx);

	return 0;

}


/**************************************************************************/
/*   cs8900_reset( ): software reset the CS8900 chip                          */
/**************************************************************************/
int cs8900_reset(void) {
    int j;
	unsigned short status;

	/* Reset chip */
	 WritePPRegister(PP_SelfCtl, (unsigned short)PP_SelfCtl_Reset);  

	 /* Wait about 125 msec for chip resetting in progress */
	 for (j=0; j<100; j++){
       DelayForAWhile();
	 }

	 /* check the PP_SelfStat_InitD bit if the chip successflly reset */
	 for (j=0; j<3000; j++) {
	   status=(ReadPPRegister(PP_SelfStat) & PP_SelfStat_InitD);
	   if ( status != 0 ) {
          return 0  /* successful */;
	   }	   
	 }
 	
	 return -1; /* fail */
}

/**************************************************************************/
/* cs8900_poll_send():  Transmit a frame in Poll mode.                    */
/**************************************************************************/
int cs8900_poll_send(unsigned char *dataBuf, int total_len)
{
    int len;
    unsigned short *sdata;
    unsigned short stat;
    unsigned long i;	


   /****** Step 0: First thing to do is to disable all interrupts at processor level.*/
	/* These steps: Write the TX command, Bid for Tx, and copy Tx data to chip must be atomic.
	   Otherwise, CS8900 may hang if interrupts occurs in the middle of these steps.*/
   /* Since the program runs in real DOS mode and CS8900's interrupt is not enabled,
	  we don't need the step 0 here.*/ 
	/* disable();*/
	
   /****** Step 1:  Write the TX command */
     if ( gPrevTxBidFail ) 
	{
        /* Previous BidForTX has reserved the Tx FIFO on CS8900, must clear off it.*/
		/* Setting PP_TxCmd_Force bit will cause CS8900 to release the previous reserved Tx buffer.*/
       outport(CS8900_TxCMD, (unsigned short) PP_TxCmd_TxStart_Full | PP_TxCmd_Force);
       gPrevTxBidFail=0;
    } 
	else 
	{
       outport(CS8900_TxCMD, (unsigned short) PP_TxCmd_TxStart_Full);
    }


	/****** Step 2: Bid for Tx ******/
	/* Write the frame length  (number of bytes to TX) */
	/* Note: After the frame length has been written, CS8900 reserves Tx buffer for this bid no
	         matter PP_BusStat_TxRDY is set or not.*/
	outport(CS8900_TxLEN, total_len);
 
	/* Read BusST to verify it is set as Rdy4Tx. */
    stat = ReadPPRegister(PP_BusStat);


	/****** Step 3: check if TxBidErr happens ******/
    if ( stat & PP_BusStat_TxBid ) 
	{    /*TxBidErr happens only if Tx_length is too small or too large. */
         printf("cs8900_poll_send(): Tx Bid Error! TxLEN=%d \n", total_len);


		 /****** Step 3.1: enable interrupts at processor level if it is disabled in step 0.******/
         /*enable( );   */

	  	 return -1;
	}
    

	/****** Step 4: check if chip is ready for Tx now ******/
      /* If Bid4Tx not ready, skip the frame */
	 if ( (stat & PP_BusStat_TxRDY) == 0)
	{
		 /* If not ready for Tx now, abort this frame.*/
         /* Note: Another alternative is to poll PP_BusStat_TxRDY until it is set, if you don't want to
		          abort Tx frames.*/ 
         gPrevTxBidFail=1;/* set to 1, and next time if cs8900_poll_send() is called, it can 
						  set PP_TxCmd_Force to clear off the reserved Tx buffer in CS8900 chip.*/

         printf("cs8900_poll_send(): Tx Bid Not Ready4Tx! TxLEN=%d \n", total_len);
		 
		 /****** Step 4.1: enable interrupts at processor level if it is disabled in step 0.******/
         /*enable( );   */

         return -1;
    }       
     
	/****** Step 5: copy Tx data into CS8900's buffer*/
    /* This actually starts the Txmit*/
    sdata = (unsigned short *)dataBuf;
    len = total_len;
    if (len > 0) {
        /* Output contiguous words, two bytes at a time. */
        while (len > 1) 
		{
           outport(CS8900_RTDATA, *sdata);
			sdata++;
            len -= 2;
        }

        /* If Odd bytes, copy the last one byte to chip.*/
        if (len == 1) 
		{
           outport(CS8900_RTDATA, (*sdata) & 0x00ff);
        }
    }

    /****** Step 5.1: enable interrupts at processor level if it is disabled in step 0.******/
    /*enable( );   */


	/****** Step 6: Poll the TxEvent Reg for the TX completed status */
	/* This step is optional. If you don't wait until Tx compelete, the next cs8900_poll_send() 
	   Bid For Tx may encounter Not Ready For Tx because CS8900 is still Tx'ing.*/
    for ( i=0; i<30000; i++) 
	{
        stat = ReadPPRegister(PP_TER);
        if ( stat != 0x0008 ) 
		{
            break;
        }
    }

	/* Tx compelete without error, return total_length Tx'ed */
	if ( (stat & PP_TER_TxOK) || (stat == 0x0008) ) {
            return total_len; /* return successful*/
	} 

	/* Tx with Errors */
	printf("cs8900_poll_send(): Tx Error! TxEvent=%x \n", stat);
	return -1; /* return failed*/

}


/**************************************************************************************/
/* cs8900_poll_recv(): can be used to receive a frame in  Poll mode.                  */
/**************************************************************************************/
int cs8900_poll_recv( unsigned char *dataBuf)
{
    unsigned short stat, totalLen, val;
    int leftLen;
    unsigned short *data;
    unsigned char *cp;

    /****** Step 1: check RxEvent Register ******/
	stat = ReadPPRegister(PP_RER);

    /****** Step 2: Determine if there is Rx event. ******/
	/* If nothing happened, then return. 0x0004 is the register ID.
	   If some bits in Bit 6 - Bit 15 are set, some Rx event happened. */
	if ( stat == 0x0004 ) {
		return 0;
	}


    /****** Step 3: Determine if there is Rx Error. ******/
	/* If RxOk bit is not set, Rx Error occurred  */
	if ( !(stat & PP_RER_RxOK) ) 
	{
		if (  stat & PP_RER_CRC) 
		{
 		    printf("cs8900_poll_recv(): received frame with CRC error.\n"); 
		} 
		else if ( stat & PP_RER_RUNT) 
		{
			printf("cs8900_poll_recv(): received frame with RUNT error.\n"); 
		}
		else  if (stat & PP_RER_EXTRA ) 
		{
			printf("cs8900_poll_recv(): received frame with EXTRA error.\n"); 
		}
		else 
			printf("cs8900_poll_recv(): Unknown Error: stat=%x \n", stat);

         /****** Step 4: skip this received error frame. ******/
         /* Note: Must skip this received error frame. Otherwise, CS8900 hangs here. */
		/* Read the length of Rx frame */
        ReadPPRegister(PP_Rx_LENGTH);
		/* Write Skip1 to RxCfg Register and also keep the current configuration.*/
		val = ReadPPRegister(PP_RxCFG);
		val |= PP_RxCFG_Skip1; 
	    WritePPRegister(PP_RxCFG, val);

		return -1; /* return failed */
    }

 	 /****** Step 5: Read the length of Rx frame ******/
     totalLen = ReadPPRegister(PP_Rx_LENGTH);

	 /* uncomment printf for debugger purpose.*/
	/*printf("RxEvent - stat: %x, len: %d\n", stat, len);*/

     
 	 /****** Step 6: Read the Rx data from Chip and store it to user buffer ******/
    data = (unsigned short *)dataBuf;
    leftLen = totalLen;

	/* read 2 bytes at a time */
    while (leftLen >= 2) {
			*data++ = inport(CS8900_RTDATA);
          leftLen -= 2;
    }

	/* if odd bytes, read the last byte from chip*/
    if (leftLen == 1) {

		/* Read the last byte from chip*/
		val = inport(CS8900_RTDATA);

        /* point to the last one byte of the user buffer*/
        cp = (unsigned char *)data;

	   /* truncate the word (2-bytes) read from chip to one byte */
       *cp = (unsigned char)(val & 0xff);
    }

	return (int)totalLen;
}



/**************************************************************************/
/*   cs8900_interrupt_init( ): start up CS8900 for Interrupt Mode         */
/**************************************************************************/
int cs8900_interrupt_init(unsigned short duplexMode){

	unsigned short val;

	/****** Step 1: start up CS8900 except Interrupt registers.******/
	cs8900_poll_init(duplexMode);

	/******* Step 2: Set the IRQ number in the chip ******/
	/* The default Interrupt line is IRQ10 */
	/* 0:IRQ10; 1:IRQ11 2:IRQ12 3:IRQ5 */
	if ( gIrqNo ==  5 ) {
      val = 3; 
    } else {
	  val = gIrqNo - 10;
	}
    WritePPRegister(PP_IntReg, val); /* 0:IRQ10; 1:IRQ11 2:IRQ12 3:IRQ5 */

	/******* Step 3: initialize the RxCFG register for Rx Event Interrupt  ******/
	 /* Accept good and bad Rx frames */
     WritePPRegister(PP_RxCFG, PP_RxCFG_RxOK | PP_RxCFG_CRC | PP_RxCFG_RUNT |PP_RxCFG_EXTRA); 

	/******* Step 3: initialize the RxCFG register for Rx Event Interrupt  ******/
    /* Or just accept good Rx frames and don't need Rx statistics for bad ones. */
	 /* WritePPRegister(PP_RxCFG, PP_RxCFG_RxOK ); */

	 /******* Step 4: initialize the TxCFG register for Tx Event Interrupt  ******/
	 /* Enable all Tx IEs. Please refer to cs8900.h for what are enabled. */
     WritePPRegister(PP_TxCFG, PP_TxCFG_ALL_IE); 

	 /******* Step 5: Enable interrupt when ready for Tx*/
	 WritePPRegister(PP_BufCFG, PP_BufCFG_TxRDY);

	 /******* Step 6: Enable CS8900's Interrupt */
     cs8900_EnableInt();

	return 0;
}



/**************************************************************************************/
/* cs8900_interrupt_recv():  receive a frame in  Interrupt mode.                  */
/**************************************************************************************/
int cs8900_interrupt_recv( unsigned char *dataBuf, unsigned short RxEventStatus)
{
    unsigned short  totalLen, val;
    int leftLen;
    unsigned short *data;
    unsigned char *cp;



    /****** Step 1: Determine if there is Rx Error. ******/
	/* If RxOk bit is not set, Rx Error occurred  */
	if ( !(RxEventStatus & PP_RER_RxOK) ) 
	{
		if (  RxEventStatus & PP_RER_CRC) 
		{
 		    printf("cs8900_interrupt_recv(): received frame with CRC error.\n"); 
		} 
		else if ( RxEventStatus & PP_RER_RUNT) 
		{
			printf("cs8900_interrupt_recv(): received frame with RUNT error.\n"); 
		}
		else  if (RxEventStatus & PP_RER_EXTRA ) 
		{
			printf("cs8900_interrupt_recv(): received frame with EXTRA error.\n"); 
		}
		else 
			printf("cs8900_interrupt_recv(): Unknown Error: stat=%x \n", RxEventStatus);

         /****** Step 2: skip this received error frame. ******/
         /* Note: Must skip this received error frame. Otherwise, CS8900 hangs here. */
		/* Read the length of Rx frame */
        ReadPPRegister(PP_Rx_LENGTH);
		/* Write Skip to RxCfg Register and also keep the current configuration.*/
		val = ReadPPRegister(PP_RxCFG);
		val |= PP_RxCFG_Skip1; 
	    WritePPRegister(PP_RxCFG, val);

		return -1; /* return failed */
    }

 	 /****** Step 3: Read the length of Rx frame ******/
     totalLen = ReadPPRegister(PP_Rx_LENGTH);

	 /* uncomment printf for debugger purpose.*/
	/*printf("RxEvent - stat: %x, len: %d\n", stat, len);*/

     
 	 /****** Step 4: Read the Rx data from Chip and store it to user buffer ******/
    data = (unsigned short *)dataBuf;
    leftLen = totalLen;

	/* read 2 bytes at a time */
    while (leftLen >= 2) {
         *data++ = inport(CS8900_RTDATA);
          leftLen -= 2;
    }

	/* if odd bytes, read the last byte from chip*/
    if (leftLen == 1) {

		/* Read the last byte from chip*/
		val = inport(CS8900_RTDATA);

        /* point to the last one byte of the user buffer*/
        cp = (unsigned char *)data;

	   /* truncate the word (2-bytes) read from chip to one byte */
       *cp = (unsigned char)(val & 0xff);
    }

	return (int)totalLen;
}



/**************************************************************************/
/* cs8900_interrupt_send():  Transmit a frame in Interrupt mode.          */
/**************************************************************************/
int  cs8900_interrupt_send(unsigned char *dataBuf, int total_len){
 
    unsigned short stat;

    /****** Step 0: First thing to do is to disable all interrupts at processor level.*/
	/* These steps: Write the TX command, Bid for Tx, and copy Tx data to chip must be atomic.
	   Otherwise, CS8900 may hang if interrupts occurs in the middle of these steps.*/
	disable();

	/* Tx is in process now */
	gTxInProgress = 1;


   /****** Step 1:  Write the TX command */
   outport(CS8900_TxCMD, (unsigned short) PP_TxCmd_TxStart_Full);


	/****** Step 2: Bid for Tx ******/
	/* Write the frame length  (number of bytes to TX) */
	/* Note: After the frame length has been written, CS8900 reserves Tx buffer for this bid no
	         motter PP_BusStat_TxRDY is set or not.*/
	outport(CS8900_TxLEN, total_len);
 
	/* Read BusST to verify it is set as Rdy4TxNow. */
    stat = ReadPPRegister(PP_BusStat);


	/****** Step 3: check if TxBidErr happens ******/
    if ( stat & PP_BusStat_TxBid ) 
	{    /*TxBidErr happens only if Tx_length is too small or too large. */
         printf("cs8900_poll_send(): Tx Bid Error! TxLEN=%d \n", total_len);

	    /* Abort the Tx frame.  set the Tx flag is not in process.*/
	   gTxInProgress = 0;

		/****** Step 3.1: enable interrupts at processor level.******/
	    enable();
   
	  	 return -1;
	}
    

	/****** Step 4: check if chip is ready for Tx ******/
	 if ( (stat & PP_BusStat_TxRDY) == 0)
	{
        /* If Bid4Tx not ready, return immediately and wait for ReadyForTx Interrupt. */

		/****** Step 4.1: enable interrupts at processor level.******/
	    enable();

         return 0;
    }       
     
	/****** Step 5: copy Tx data into CS8900's buffer*/
    /* This actually starts the Txmit*/
	CopyTxFrameToChip(dataBuf, total_len);
	
	/*enable interrupts at processor level*/
	enable();

	/* Return and wait for TxEvent Interrupt */
	return 0;

}

/**************************************************************************/
/* CopyTxFrameToChip():  copy Tx data from user buffer to CS8900 chip        */
/**************************************************************************/
void CopyTxFrameToChip(unsigned char *dataBuf, int total_len){
	int len;
    unsigned short *sdata;

     sdata = (unsigned short *)dataBuf;
    len = total_len;
    if (len > 0) {
        /* Output contiguous words, two bytes at a time. */
        while (len > 1) 
		{
           outport(CS8900_RTDATA, *sdata);
			sdata++;
            len -= 2;
        }

        /* If Odd bytes, copy the last one byte to chip.*/
        if (len == 1) 
		{
           outport(CS8900_RTDATA, (*sdata) & 0x00ff);
        }
    }

	/* return and wait for TxEvent Interrupt */
}


/******************************************************************************
*
* ProcessTxStatus()
* Purpose:  When Tx is compolete, check the Tx status.
*
******************************************************************************/
int ProcessTxStatus(unsigned short TxEventStatus){

	/* Clear Tx flag.  Tx is compelete.*/
	gTxInProgress = 0;

	/* Tx compelete without error */
	if ( (TxEventStatus & PP_TER_TxOK) || (TxEventStatus == 0x0008) ) {
            return 0; /* return successful*/
	} 

	/* Tx with Errors */
	printf(" ProcessTxStatus(): Tx Error! TxEvent=%x \n", TxEventStatus);
	return -1; /* return failed*/

}


/******************************************************************************
*
* cs8900_EnableInt()
* Purpose:  Enable CS8900 interrupt 
* return: none;
*
******************************************************************************/
void cs8900_EnableInt(void) {
	unsigned short val;

	/* Read value from BusCTL regioster */
	val=ReadPPRegister(PP_BusCtl);

	/*  Set the bit PP_BusCtl_EnableIRQ*/
	val |= PP_BusCtl_EnableIRQ;

	/* Write back to BusCTL register */
	WritePPRegister(PP_BusCtl, val);
}


/******************************************************************************
*
* cs8900_DisableInt()
* Purpose:  Disable CS8900 interrupt
* return: none;
*
******************************************************************************/
void cs8900_DisableInt(void) {
	unsigned short val;

	/* Read value from BusCTL regioster */
	val=ReadPPRegister(PP_BusCtl);

	/*  Clear the bit PP_BusCtl_EnableIRQ*/
	val &= ~PP_BusCtl_EnableIRQ;

	/* Write back to BusCTL register */
	WritePPRegister(PP_BusCtl, val);
}

/******************************************************************************
*
* cs8900_ISR()
* Purpose: CS8900 interrupt handler.
* return: none;
*
******************************************************************************/
void interrupt cs8900_ISR() {

    unsigned short IntStatus;
	int stat;


   /****** Step 1: Read and clear the interrupt status ******/
    IntStatus =  inport(CS8900_ISQ);
    if ( !IntStatus) {
        IntStatus =  inport(CS8900_ISQ); /*If not set, read ISQ directly again*/
	}

	/* The following code is for IBM compatible PC*/
	outportb( PICirr, 0x20 ); /*EOI=0x20*/ /* non-zero event drops IRQ */
	if ( PICirr != 0x20 )
	{
		outportb( 0x20, 0x20 ); /*EOI=0x20*//* PIC 1 also */
	}

 
	/* Step 2: a loop to read ISQ Register and process events until no event left. */
    while ( IntStatus != 0 )
    {
        switch ( IntStatus & ISQ_EventMask ) 
        {
	    case ISQ_RxEvent:
			    /* A frame is Rx'ed.  Check and process it. */
                stat=cs8900_interrupt_recv(gRxDataBuf, IntStatus);

				/* update statistics */
				if ( stat == -1 ) {
				   /* Rx with error */
                   gIntRxErr++;
				} else if ( stat > 0 ) {
				  /* Rx without error */
                  gIntRxOK++;
				}
                break;

	    case ISQ_TxEvent:
			    /* Tx compelete. Check the Tx status */
                stat=ProcessTxStatus(IntStatus);

				/* update statistics */
				if ( stat == -1 ) {
				  /* Tx with error */
                  gIntTxErr++;
				} else {
				  /* Tx without error */
                  gIntTxOK++;
				}
                break;

	    case ISQ_BufEvent:
			    /* CS8900 chip has reserved Tx FIFO in the last bid for Tx. It is ready for Tx now.
			       So, copy Tx data to cs8900 chip.*/
			    if (IntStatus & PP_BufEvent_Rdy4Tx) {
 				  	if ( gTxInProgress == 1 ) {
                        CopyTxFrameToChip( gTxDataBuf, gTxLength);
					} else {
                      /* CS8900 chip does NOT reserve Tx FIFO. The Rdy4Tx event should not happen.*/
					  printf("cs8900_ISR(): Error: PP_BufEvent_Rdy4Tx is set but Tx is not in progress.");
					}
				}
                break;

	    case ISQ_RxMissEvent: /* counter-overflow report: RxMISS (register 10)*/
			    /* Do your statistics here.*/
                break;

	    case ISQ_TxColEvent: /* counter-overflow report: TxCOL (register 12)*/
			    /* Do your statistics here.*/
                break;

        }

        /* Read and clear the interrupt status again */
        IntStatus = inport(CS8900_ISQ);
    } /* end while */
}

/****************************************************************************
 * PIC initialization
 * 20 Init Control Word One (bit 4 is 1)
 *   3 0=Edge triggered, 2 0=8-bytes per interrupt vector, 1 0=cascade mode
 *   0 0=No ICW4 is needed. Default is 00
 * 21 ICW2 7-3 A0-A3 of base vector address, 2-0 reserved
 * 21 ICW3 1 if slave controller attached to pin
 * A1 ICW3 7-3 reserved, 2-0 Slave ID
 * 21 OCW2 (4,3 both 0) 765=001 Non-specific EOI, 2-0 Interrupt to which EOI
 *         applies
 * Phoenix System BIOS, 2nd Edition p71-73
 *****************************************************************************/
void hookint( unsigned irqno, int install_int )
{
    unsigned intno;
	unsigned char PICbyte;

    intno = irqno;


    if ( install_int )
    {
		if ( intno == 5 )
		{
            PICirr = 0x20;
			PICimr = 0x21;
			PICint = 1 << intno;
            PICmask = ~PICint;
            intno += 8;                        /* IRQ  5 uses vector  D */
        }
        else
        {
            PICirr = 0xA0;
			PICimr = 0xA1;
            PICint = 1 << ( intno - 8 );
            PICmask = ~PICint;
			intno += 0x68;                     /* IRQ 10 uses vector 0x72, IRQ11:0x73; IRQ12:0x74;*/
        }

 
		gOldIntHandlerPtr = getvect(intno );     /* save existing system vector */

        disable( );                            /* disable interrupts */
                                               /* new mask to PIC */
        setvect( intno, cs8900_ISR );
        PICbyte = inportb( PICimr );
        PICbyte &= PICmask;

		/* Do nothing. Delay for a while */
	  	DelayForAWhile();


		outportb( PICimr, PICbyte );

		enable( ); /* enable interrupts */

    }
    else
    {
		while ( inport(CS8900_ISQ ) );              /* drain CS8900 ISQ directly */

        if ( intno == 5 )
        {
            intno += 8;                        /* IRQ  5 uses vector  D */
		}
		else
    	{
            intno += 0x68;                     /* IRQ 10 uses vector 72 */
        }

		/* Disable interrupt at processor level */
        disable( );                            /* disable interrupts */
                                               /* new mask to PIC */
		PICbyte = inportb( PICimr );
        PICbyte |= PICint;

		/* Do nothing. Delay for a while */
		DelayForAWhile();

		outportb( PICimr, PICbyte );
        setvect( intno, gOldIntHandlerPtr );      /* restore system vector */
        enable( );                             /* enable interrupts */
	}

}/* hookint */


/*****************************************************************************
 * ISA I/O instructions take ~1.0 microseconds. Reading the NMI Status
 * Register (0x61) is a good way to pause on all machines.
 *****************************************************************************/		 
void DelayForAWhile(void){

          inportb(0x61);
          inportb(0x61);

}

