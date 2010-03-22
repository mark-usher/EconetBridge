
The CS8900 demonstration program is a DOS-based program targeted for x86 
platforms.  The purpose of the program is to illustrate CS8900's operation.
CS8900 OEM customers can use this program as the reference code to implement
their own drivers.

The CS8900 driver for VxWorks gives a good example for what a real Ethernet
driver looks like.  That driver supports both Poll and Interrupt modes, and 
can be downloaded form Cirrus Logic's Ethernet Web page:
http://www.cirrus.com/drivers/ethernet/ethernet.cfm


The compressed zip file of the CS8900 demonstration program contains the 
following files:

1. ReadMe.txt - this file. It contains the sections of User Guide, Porting
                Guide, implement your own CS8900 driver in Polling and implement
                 your own CS8900 driver in Interrupt Mode.
2. demo.c - C source code of the main function.
3. cs8900.c - C source code of the cs8900 functions.
4. cs8900.h - head file for cs8900.
5. demo8900.ide - the project file to build the executable image under 
                  Borland C/C++ 4.52.
6. demo8900.exe - the executable image of the CS8900 demonstration program 
                  under DOS.


=====================================================================
User Guide for the CS8900 demonstration program
=====================================================================

The following steps describe how to run this program under DOS.

If your CS8900 network card is with default setting, you can skip 
steps 3 to 5. 


1. Plug the CS8900 adapter to an IBM compatible PC, and connect the RJ-45 
   port to a hub.

2. Boot PC by DOS floppy (DOS 5.0, 6.2, or others) in drive A.

3. Run setup.exe (in Windows_Networking_SWDiskV310.zip, or newer version
   can be downloaded from Cirrus Logic's Ethernet Web page: 
   http://www.cirrus.com/drivers/ethernet/ethernet.cfm)
   to set CS8900's configuration to IO port 0x300, IRQ 10, RJ-45, and
   half duplex.

4. Power off and power on the PC.  You must cold reset the PC.  Otherwise,
   CS8900 won't reload the new settings.

5. Boot PC by DOS floppy in drive A.

6. Run demo8900.exe by typing:

   A:\demo8900  

   and press [Enter].

7. After the demo8900.exe has started,  press [Enter] to transmit 1000 frames 
   with the length of 1513 bytes in Polling Mode.  While transmitting, CS8900
   also receives all frames that appear in the network cable.  The transmission
   and reception statistics will display on the screen.

8. Press [Enter] to transmit another 1000 frames with the length of 1513 bytes
   in Interrupt Mode.  While transmitting, CS8900 also receives all frames which 
   appear in the network cable.  The transmission and reception statistics will 
   display on the screen.


=====================================================================
Porting Guide for the CS8900 demonstration program
=====================================================================

The default settings in this program are
IO Port: 0x300, IRQ 10, 10BaseT, half-duplex.

You can change the default setting by modifying the following items in the
source code.

In cs8900.h
#define  CS8900_BASE 0x300 /* Default IO Port Address */

In demo.c
int gTxLength=1513; /* Always Tx 1513-byte-long frame */

unsigned short gDuplexMode=0; /* default: 0:half-duplex;  0x4000=Full-duplex*/

/* The CS8900's IRQ number.  It can be 5, 10, 11, and 12. */
unsigned int gIrqNo=10;



The program is developed under Borland C/C++ 4.52.  If you want to re-build the
Program by using other C compilers that support Real DOS mode, the following
functions in dos.h and conio.h may need porting:

/* write the 16-bit-wide value to the IO port number port_number */
void outport(unsigned port_number, unsigned value);

/* read the 16-bit-wide value from the IO port number port_number */
unsigned inport(unsigned port_number);

/* write the 8-bit-wide value to the IO port number port_number */
void outportb(unsigned port_number, unsigned char value);

/* read the 8-bit-wide value from the IO port number port_number */
unsigned char inportb(unsigned port_number);


If the part for Interrupt Mode demonstration needs to be re-built, the
Following functions may need porting:

void enable(void); /* enable interrupts at processor level*/

void disable(void); /* disable interrupts at processor level*/

/* Get the interrupt handler of the interrupt_number */
void interrupt getvect(unsigned interrupt_number ); 

/* Set the interrupt handler for the interrupt_number */
void setvect(unsigned interrupt_number, void interrupt (*FunPointer) ); 


=====================================================================
Implement your own CS8900 driver in Polling Mode
=====================================================================

The section gives information of how to implement your own CS8900 driver
in Polling Mode.  The default settings in this program are IO Port: 0x300, 
RJ-45, half-duplex, and IO Port mode(not memory mode). To simplify the
demo8900 program, only these default settings are discussed. Please refer to 
the CS8900 data sheet for other configurations.


The Ethernet driver's operation in Polling mode can be divided 
into three procedures:  Start-Up, Transmit, and Receive.

1. Start-Up
------------

   CS8900 must be configured for packet transmission and reception at power-up 
   or reset.  It is done during the Start-Up procedure.
 
   - Step 1: software reset the chip

   Reset CS8900 by enabling the SelfCtl_Reset bit in the SelfCtl register.
   Wait about 125 msec for chip resetting in progress.
   Check the SelfStat_InitD bit in the SelfStat register if the chip 
   has successfully reset.

   It is important to always reset the CS8900 chip when the driver is loaded.  
   Otherwise, the CS8900 chip may hang because it is in the middle of some
   operation.


   - Step 2: Set up Ethernet hardware address
  
   Write the 6-byte-long Ethernet hardware address to Individual Address
   Register two bytes at a time.

   The Ethernet hardware address, serial number, IO Port number, and other 
   information can be stored in EEPROM.  


   - Step 3: Configure RxCTL Register to receive frames.

   Set RxCTL_RxOK Bit to receive good frames.

   Usually, an Ethernet driver accepts Individual Address, Broadcast, and 
   Multicast frames. In the demo8900 program, it turns on the Promiscuous mode
   to receive all network traffic. This is to demonstrate the reception
   capability of the CS8900 chip.
    
   - Step 4: Configure TestCTL for Duplex Mode)
   
   Set Half or Full duplex mode in TestCTL Register.  The default is half-duplex
   after chip reset.


- Step 5: enable Serial Transmission and Serial Reception in LineCTL   Register
 
 
  Now, the cs8900 chip is ready to transmit and receive packets in Polling mode.


2. Transmit
-------------

   Refer to Figure 30 (page 102) in CS8900 Data Sheet for the Tx flow char.

   Packet transmission occurs in two phase.  In the first phase, the driver
   writes the TX command, bids for Tx, and copies Tx data to CS8900 chip. 
   These three steps must be atomic. Otherwise, CS8900 may hang if interrupts
   occur in the middle of these steps.  In the second phase, CS8900 chip 
   transmits the Ethernet frame onto the network and reports the Tx status. 
   The driver can poll the TxEvent Register for waiting for Tx completion and o 
   checking the Tx status.

   First thing to prepare CS8900 for the transmission is to disable all 
   interrupts at processor level. Since the steps in the first phase should be
   atomic, disable interrupts can make sure this.

   - Step 1:  Write the TX command

     Tells CS8900 chip when to start transmission (i.e. after 5, 381, 1021 or
     all bytes have been copied to CS8900's internal FIFO.)  We suggest to use
     TxCmd_TxStart_Full first. This setting is the most inefficient but the most
     reliable.  If you have a fast processor, try TxCmd_TxStart_1021, 
     TxCmd_TxStart_381, and TxCmd_TxStart_5 sequentially.  If the processor
     is not fast enough to copy Tx data into CS8900's FIFO, a Tx UnderRun error
     occurs. A network protocol analyzer (Sniffer) can be used to capture 
     packets with Tx UnderRun error.  The packet with Tx UnderRun error has CRC
     error (because it is not completely Tx'ed) and its length is shorter than
     expected.

  
  - Step 2: Bid for Tx 

	Write the frame length (number of bytes to TX) to the TxLength Port.  The
      Tx Length should less than or equal to 1514 and larger than or equal to 16
      bytes.  CS8900 will calculate the 32 bit CRC and appended it to the frame.
      If the Tx length is less than 60 bytes, CS8900 chip will pad bytes so the
      actual Tx length is still 60 bytes.

  - Step 3: check if TxBidErr happens 

    Read BusST Register to check if Tx Bid Error happens. If the Tx Length is
    less than 16 bytes or larger than 1514 bytes, the TxBidErr occurs.

  - Step 4: check if chip is ready for Tx 

    Check the status read from BusST Register to determine if CS8900 has FIFO
    available and is ready for Tx.

    If BusStat_TxRDY is not set, there are two alternatives.  Which alternative
    to use is dependent on your OS.

1. Abort the Tx frame immediately : enable interrupt, and return with error
   or without error. 

   The best solution is that the driver reports DEVICE-BUSY and the OS 
   provides the scheme to re-transmit the frame later. If the driver returns
   without any error, the upper-layer network protocol will recover the lost
   frame but performance degrades a lot. If the driver returns with 
   NETWORK-ERROR, be careful, the OS may shut down the network interface of
   CS8900.


    2. Poll BusStat_TxRDY: 
       Poll the TxRDY bit in BusStat Register until the TxRDY bit is set or time
       out.  If the network traffic is heavy, many collisions may occur as
       CS8900 tries to transmit frames onto the network cable. If the previous
       frame in CS8900's FIFO is not transmitted completely, CS8900
       won't accept the next frame for Tx. Therefore, the driver may experience
       in polling BusStat_TxRDY for a long time. Since the interrupt at 
       processor level is disabled, the time to poll the TxRDY bit must be
       limited and short.  Otherwise, the driver may block other OS and device
       tasks.

       If timed out, the driver must abort the Tx frame and return with error or
       without error. Please see the first alternative for the pros and cons.

 If the driver returns after Step 4, the globe flag, gPrevTxBidFail, must be set
 to true.  Since the Tx command and Tx length have been written to CS8900,
 CS8900 has reserved the Tx buffer for the Tx data.  CS8900 expects the driver
 to copy the Tx data into its FIFO in next step. Hence, the driver should tell
 CS8900 to discard the reserved Tx FIFO in the next transmission. Otherwise,
 CS8900 hangs because it is in the wrong state. Therefore, in the reference
 code, the driver checks gPrevTxBidFail before it issues the Tx command.  If the
 flag is true, the TxCmd_Force bit is set to force CS8900 to discard its
 previously reserved Tx FIFO.


  - Step 5: copy Tx data into CS8900's FIFO

    Copy Tx data from user buffer to CS8900's FIFO two bytes at a time. If the
    Tx length is odd, put the last byte of Tx data into the lower byte of a word
    (two bytes) and make the higher byte of the word 0.  Write the word to 
    CS8900.


 This is the end of the first phase. Enable interrupts at processor level. If
 the Tx statistics are not calculated, the driver can return without error here.
 If the driver doesn't wait until the Tx complete, usually, a global flag 
 (TxNotCompelete) is used to indicate that the Tx is still in progress.
 Thus, the driver has to check the TxNotCompelete flag every time before it
 writes the Tx command to CS8900. If the flag is true, the driver should poll
 the Tx status until the Tx complete.  The Tx FIFO in CS8900 is always 
 unavailable for next transmission while CS8900 is transmitting. Thus, the bid
 for Tx in Step 2 will fail.


  - Step 6:  Poll the TxEvent Register for the TX completed status 

    This step is optional because the driver may return immediately after 
    Step 5. Since the processor interrupt has been enabled before Step 6,
    polling Tx status won't block other OS and device tasks.  We suggest that
    perform this step and make your driver easy to follow.


3. Receive
------------

   The driver polls the RxEvent Register.  If a frame is received, the driver
   transfers the Rx data from CS8900 chip into the user Rx buffer.
   
   - Step 1: Read RxEvent Register

     The Rx events in RxEvent Register is cleared after the register is read.
     Therefor, just read once and keep a copy of the Rx event.

   - Step 2: Determine if there is Rx event. 
   
     If the value read is only register ID (0x0004), nothing happens. The driver 
     returns immediately. Otherwise, a frame is received and the driver should
     process the Rx event.

   - Step 3: Determine if there is Rx Error.
 
     If RxOk bit is not set, Rx Error occurs. Do your Rx error statistics then. 

   - Step 4: If there is Rx Error, skip this received error frame. 

     Must skip this received error frame. Otherwise, CS8900 is in the state
     waiting the driver to read the Rx length and the received frame data from 
     its register.  Thus, CS8900 hangs for next Tx or Rx.

     Read the length of Rx frame and set the Skip1 bit in RxCfg Register to skip
     the received frame.

  - Step 5: If there is no Rx Error, Read the length of Rx frame.

  - Step 6: Read the Rx data from CS8900 chip and store it to user buffer

    Transfer Rx data from CS8900's FIFO into user buffer two bytes at a time. 
    If the Rx length is odd, read a word (two bytes) for the last byte from 
    CS8900 but only copy the lower byte of the word to User buffer.
 

 
=====================================================================
Implement your own CS8900 driver in Interrupt Mode
=====================================================================

The section gives information of how to implement your own CS8900 driver
in Interrupt Mode.  The default settings in this program are IO Port: 0x300, 
IRQ 10, RJ-45, half-duplex, and IO Port mode(not memory mode). To simplify the
demo8900 program, only these default settings are discussed. Please refer to 
the CS8900 data sheet for other configurations.


The Ethernet driver's operation in Interrupt mode can be divided 
into four procedures:  Start-Up,  Transmit Receive, and Interrupt Service Routine.
		

1. Start-Up
------------

   Please read the start-up section in Poll mode thoroughly.  The steps to start
   up CS8900 in Interrupt mode is the same steps to start up CS8900 in Poll mode
   plus initializing interrupt registers.
 
  - Step 1: start up CS8900 just like in Poll mode

    Please read the start-up section in Poll mode thoroughly.

  - Step 2: Set the IRQ number in the Interrupt Register

    The default Interrupt line is IRQ10. Please refer to Table 1 in CS8900 Data
    Sheet for interrupt assignments.

  - Step 3: initialize the RxCFG register for Rx Event Interrupt 

    If Rx error statistics are needed, the driver configures CS8900 to accept
    both good and bad Rx frames.  Otherwise, just accept good frames.

  - Step 4: initialize the TxCFG register for Tx Event Interrupt 

    Enable all Tx Interrupt Events, Tx with error or without error.  Even though
    Tx statistics may not be required, the driver needs to know when the Tx has
    completed so that the driver can clear the Tx flag and send next Tx frame.

  - Step 5: initialize the BufferConfiguration Register

    Enable interrupt when ready for Tx. 

  - Step 6: Enable CS8900's Interrupt

    Set the EnableIRQ bit in TestCTL Register.


2. Transmit
------------

   Refer to Figure 31 (page 104) in CS8900 Data Sheet for the Tx flow char.
 
   Please read the Transmit section in Polling mode thoroughly.  Here, we just
   discuss the difference between Polling mode and Interrupt mode.

   The only different thing is in Step 4: check if chip is ready for Tx 
   If Bid4Tx is not set, the driver returns immediately and wait for the
   ReadyForTx interrupt. When CS8900 has Tx FIFO available, an interrupt for the
   Buffer event, ReadyForTx, is triggered.  Then, the driver copies Tx data to
   CS8900.  The phase 1 of Tx and the phase 2 are executed through
   different function calls at different time. Therefore, your driver needs some
   way to keep the address of the Tx buffer and the Tx length.


3. Receive
-----------

   Please read the Receive section in Polling mode thoroughly.  There is no
   difference between Polling Mode and Interrupt Mode.


4. Interrupt Service Routine
------------------------------

  Refer to Figure 19 (page 80) in CS 8900 Data Sheet for the flow char of
  Interrupt Service Queue.

  CS8900_ISR() is the interrupt handler for Ethernet interrupt and it should be 
  hooked up to the vector table. 

  - Step 1: Read and clear the interrupt status 
  
    Read the ISQ port for interrupt events.  The interrupt status in ISQ port
    can be read only once and is cleared after it is read.  If the interrupt
    status read is all zeros, the driver had better to read the ISQ port again.
    Sometimes, there is a delay for CS8900 to update the ISQ port.
   

  - Step 2: a loop to read ISQ Register and process events until no event left. 
  
    Process events in the order:  RxEvent, TxEvent, BufferEvent, RxMissEvent,
    and TxColEvent.

    RxEvent means that a frames is received in CS8900's buffer.  Rx event is
    always to be processed the first. Otherwise, CS8900 may run out of Rx buffer
    and drops Rx frames.

    TxEvent means that the transmission has been complete.  The driver can clear
    the global Tx flag and perform Tx statistics.

    BufferEvent means that CS8900 chip have Tx FIFO available and is ready for
    Tx now. The BufferEvent only happens after the driver's bid for Tx has 
    failed.  Every time the driver bids for Tx, no matter what is the result,
    CS8900 is committed to reserve Tx buffer for the bid.  If the bid fails, the
    Driver should return immediately and wait for the BufferEvent interrupt.
    Since writing Tx command and bidding space for Tx have done when the driver
    gets the BufferEvent interrupt, the driver only 
    needs to copy Tx data to chip.

    RxMiss and TxCol events are for statistics purpose.

    

    








       
      
  










