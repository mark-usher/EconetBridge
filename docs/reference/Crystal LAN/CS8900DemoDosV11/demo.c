/*==========================================================================

File Name: Demo.c

Software Utility program for CS8900A Ethernet chip to demostrate Tx and Rx in Poll 
and Interrupt Modes.

==========================================================================
History:

  10/20/00 Melody
  -Created the program.

==========================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <conio.h>

#include "cs8900.h"


void CreateTxData(unsigned char *pTxBuffStart, unsigned short FrameLen, char *eAddr);
int InterruptModeDemo(void);
int PollModeDemo(void);

unsigned char gRxDataBuf[1520];/* buffer to hold Rx data */
unsigned char gTxDataBuf[1520];/* buffer to hold Tx data */
int gTxLength=1513; /* Always Tx 1513-byte-long frame */
unsigned short gDuplexMode=0; /* defualt: 0:half duplex;  0x4000=Full duplex*/
int  gIntTxErr=0, gIntTxOK=0, gIntRxErr=0, gIntRxOK=0; /* Tx and Rx statistics for Interrupt Mode*/
	
/* The CS8900's IRQ number.  It can be 5, 10, 11, and 12. */
unsigned int gIrqNo=10;


extern unsigned char gEtherAddr[]; /* Ethernet physical address of the chip*/
extern int gTxInProgress; /* The flag to know if Tx is in progress (not compelete) */

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
void CreateTxData(unsigned char *pTxBuffStart, unsigned short FrameLen, char *eAddr) 
{
     unsigned char *pRoller;
	 int dataCnt;
 
    pRoller=pTxBuffStart;

	 /* copy the Broadcast destination address to the frame */
    memset(pRoller, 0xff, 6);
    pRoller +=6;

    /* copy the source address to the frame */
    memcpy(pRoller, eAddr, 6);
    pRoller +=6;

	/* Set the rest of the frame to the pattern 0xA */
    dataCnt=FrameLen-12;
    memset(pRoller, 0xa, dataCnt);
 
}

/**************************************************************************/
/*   PollModeDemo( ): */
/**************************************************************************/
int PollModeDemo(void) {
    int  i, stat,  TxErr=0,   TxOK=0,   RxErr=0,   RxOK=0;
	char txbuf[80];

	/* start up CS8900 */
    stat = cs8900_poll_init(gDuplexMode);
	if (stat != 0 ) { /* cs8900 init failed */
	   printf("PollModeDemo() Error: cannot initalize CS8900!\n");
       return -1;  
	}

     /* execute the function specified by user*/
     printf("\n\nPoll Demo: \nPress [RETURN] to start transmiting 1000 frames and receiving frames while transmitting...\n");
     gets(txbuf);
        
	 for (i=0; i<1000; i++) {
		 /* Send a frame in Poll mode */
         stat=cs8900_poll_send(gTxDataBuf, gTxLength);
         if ( stat == -1 ) {
            TxErr++;
		 } else {
            TxOK++;
		 }

		/* Receive a frame in Poll mode */
	    stat=cs8900_poll_recv(gRxDataBuf);
        if ( stat == -1 ) {
           RxErr++;
		} else if ( stat > 0 ) {
           RxOK++;
		}
	 } /* end for i */

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
	 int  i, stat;
	char txbuf[80];

     /* hook interrupt service routine for CS8900 */
	 hookint(gIrqNo,1);

	 /* execute the function specified by user*/
	 printf("\n\nInterrupt Demo: \nPress [RETURN] to start transmiting 1000 frames and receiving frames while transmitting...\n");
	 gets(txbuf);

	/* start up CS8900 */
	stat = cs8900_interrupt_init(gDuplexMode);
	if (stat != 0 ) { /* cs8900 init failed */
		printf("InterruptModeDemo() Error: cannot initalize CS8900!\n");
		return -1;
	}


	for (i=0; i<1000; i++) {
		 /* Send a frame in Interrupt mode */
		stat=cs8900_interrupt_send(gTxDataBuf, gTxLength);

		/* wait until the previous Tx compelete */
        while (gTxInProgress) {};

	} /* end for i */

	printf("Interrupt Demo: Total Tx OK = %d \n", gIntTxOK);
	printf("Interrupt Demo: Total Tx Error = %d \n", gIntTxErr);
	printf("Interrupt Demo: Total Rx OK = %d \n", gIntRxOK);
	printf("Interrupt Demo: Total Rx Error = %d \n", gIntRxErr);

    /* resotore the old interrupt service routine for the IRQ number used by CS8900 */
	hookint(gIrqNo,0);

	return 0;

}

