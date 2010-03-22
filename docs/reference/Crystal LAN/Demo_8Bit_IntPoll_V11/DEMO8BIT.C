/*=======================================================================

File Name: Demo8bit.c

Software Utility program for CS8900A Ethernet chip to demostrate Tx and
Rx in 8-bit mode.

=========================================================================
History:

  3/28/02	Tanya	  Created the program.

=======================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <conio.h>

#include "cs8900.h"


void CreateTxData(unsigned char *pTxBuffStart,
		unsigned short FrameLen, char *eAddr);

int PollModeDemo(void);
int InterruptModeDemo(void);

unsigned char gRxDataBuf[1520];	// buffer to hold Rx data
unsigned char gTxDataBuf[1520];	// buffer to hold Tx data
int gTxLength=1513;		// Always Tx 1513-byte-long frame
unsigned short gDuplexMode=0;	// default: 0x0 = half duplex
				// 0x4000 = Full duplex

int gIntTxErr=0, gIntTxOK=0, gIntRxErr=0, gIntRxOK=0, gBufEvent_Rdy4Tx=0;

/* The CS8900's IRQ number.  It can be 5, 10, 11, and 12. */
unsigned int gIrqNo=10;


extern unsigned char gEtherAddr[];	// Ethernet physical address
					// of the chip

extern int gTxInProgress;		// This flag is set if Tx is
					// in progress (not compelete)

extern int gPrevTxBidFail; // This flag is set if the previous Tx bid is
					// failed.

/**************************************************************************/
/*   main( ): */
/**************************************************************************/

void main(void) 
{
    CreateTxData(gTxDataBuf, 1514, (char *)gEtherAddr);

	 PollModeDemo();
    InterruptModeDemo();
}

/**************************************************************************/
/*   CreateTxData( ): */
/**************************************************************************/
void CreateTxData(unsigned char *pTxBuffStart,
			unsigned short FrameLen, char *eAddr)
{
     unsigned char *pRoller;
     int dataCnt;

     pRoller=pTxBuffStart;

    // copy the Broadcast destination address to the frame
    memset(pRoller, 0xff, 6);
    pRoller +=6;

    // copy the source address to the frame
    memcpy(pRoller, eAddr, 6);
    pRoller +=6;

    // Set the rest of the frame to the pattern 0xA
    dataCnt=FrameLen-12;
    memset(pRoller, 0xa, dataCnt);
 
}

/*******************************************************************/
/*   PollModeDemo( ): 					           */
/*******************************************************************/
int PollModeDemo(void)
{
     int  i, stat,  TxErr=0,   TxOK=0,   RxErr=0,   RxOK=0;
     char txbuf[80];

     /* start up CS8900 */
     stat = cs8900_poll_init(gDuplexMode);
     if (stat != 0 )
     {
	// cs8900 init failed
	printf("PollModeDemo() Error: cannot initalize CS8900!\n");
	return -1;
     }

     // execute the function specified by user
     printf("\n\nPoll Demo: \nPress [RETURN] to start transmiting 1000 frames and receiving frames while transmitting...\n");
     gets(txbuf);

	  for (i=0; i<1000; i++)
     {
	// Send a frame in Poll mode
	stat=cs8900_poll_send(gTxDataBuf, gTxLength);
	if ( stat == -1 )
	{
	    TxErr++;
	}
	else
	{
	    TxOK++;
	}

	// Receive a frame in Poll mode
	stat=cs8900_poll_recv(gRxDataBuf);
	if ( stat == -1 )
	{
	   RxErr++;
	}
	else if ( stat > 0 )
	{
	   RxOK++;
	}
     } // end for i loop

     printf("Poll Demo: Total Tx OK = %d \n", TxOK);
     printf("Poll Demo: Total Tx Error = %d \n", TxErr);
     printf("Poll Demo: Total Rx OK = %d \n", RxOK);
     printf("Poll Demo: Total Rx Error = %d \n", RxErr);

     return 0;
}

/**************************************************************************/
/*   InterruptModeDemo( ): */
/**************************************************************************/
int InterruptModeDemo(void) {
	 int  stat;
	char txbuf[80];
	unsigned long i;

     /* hook interrupt service routine for CS8900 */
	 hookint(gIrqNo,1);

	 /* execute the function specified by user*/
	 printf("\n\nInterrupt Demo: \nPress [RETURN] to start transmiting frames and receiving frames while transmitting...\n");
	 gets(txbuf);

	/* start up CS8900 */
	stat = cs8900_interrupt_init(gDuplexMode);
	if (stat != 0 ) { /* cs8900 init failed */
		printf("InterruptModeDemo() Error: cannot initalize CS8900!\n");
		return -1;
	}


	//for (i=0; i<30000; i++) {
	i=0;
	for (;;) {

		 /* Before request a Tx, alwayse check if the previous Tx has compeleted.
			 If not, we have three alternatives:
			 1. Abort the Tx frame. (The simplest)
			 2. Tell OS that the etherent device is busy, retry latter on. (The best)
			 3. Wait until the previous Tx compelete. (Performance suffers)
		  */

		 /* Here shows the 3rd way: */
		/* wait until the previous Tx compelete */
		while (gTxInProgress) {};

		 /* Send a frame in Interrupt mode */
		stat=cs8900_interrupt_send(gTxDataBuf, gTxLength);


		if ( i % 2000 == 0 ) {
		  printf("\nInterrupt Demo: Total Tx OK = %d \n", gIntTxOK);
		  printf("Interrupt Demo: Total Tx Error = %d \n", gIntTxErr);
		  printf("Interrupt Demo: Total Rx OK = %d \n", gIntRxOK);
		  printf("Interrupt Demo: Total Rx Error = %d \n", gIntRxErr);
		}

		i++;

	} /* end for  */


	 /* resotore the old interrupt service routine for the IRQ number
		 used by CS8900 */
	hookint(gIrqNo,0);

	return 0;

}
