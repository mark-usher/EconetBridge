        .include "M103def.inc"
        .list
        .listmac
        .eseg
        .db      "Copyright 1998-1999 Gary T. Desrosiers. All Rights Reserved.",0
        .cseg
        .org    0
         jmp    start
;-----------------------------------------------------------------------+
;                ATmega103 <===> Crystal Interconnects.                 |
;-----------------------------------------------------------------------+
;                                                                       |
;PORTA - Unconnected. Because external SRAM enable bit set in MCUCR,    |
;		 this port becomes the data port (AD0-7).                       |
;                                                                       |
;PORTB - Connected to SA0-SA3 of Crystal board. PB4-PB7 is unconnected  |
;                                                                       |
;PORTC - Unconnected. Because external SRAM enable bit set in MCUCR,    |
;        this port becomes the high order address bus (A8-A15).         |
;                                                                       |
;PORTD - Connected to SD0-SD7 of the Crystal board (Crystal Data Bus).  |
;                                                                       |
;PORTE - Tied to PE0-->>/RD, PE1-->>/WR, PE2-->>/CHIPSEL,               |
;        and PE3<<--INTR on the Crystal board.                          |
;                                                                       |
; Note: When using the STK300 development board, remove the PORTB,      |
;       PORTD, and PORTE jumpers.                                       |
;                                                                       |
;-----------------------------------------------------------------------+
; Note: When using the STK300 development board, perform the following: |
;                                                                       |
;       * Remove the PORTB jumpers (all 8)                              |
;       * Remove the PORTD jumpers (all 8)                              |
;       * Remove PORTE jumpers for RE0/RE1                              |
;       * Move J1 to J2 to disable 3.3v mode and enable 5.0v mode       |
;       * Optionally, connect the 16x1 or 2x8 LCD                       |
;-----------------------------------------------------------------------+
;
.equ CDB=PORTD                          ;Crystal Data Bus
.equ CAB=PORTB                          ;Crystal Address Bus
.equ CSIG=PORTE                         ;Crystal Signals
.equ IOR=0                              ;Read
.equ IOW=1                              ;Write
.equ CHIPSEL=2                          ;Chip Select
.equ INTR=3                             ;Interrupt signal
;
;
;
; *Change these* : Configuration equates. 
;
.equ    ClkFreq = 4000000	            ;4 Mhz crystal
;
.equ    IP1	 =	192                     ;first octet of IP address
.equ    IP2  =  168                     ;second octet of IP address
.equ    IP3  =  1                       ;third octet of IP address
.equ    IP4  =  2                       ;fourth octet of IP address
;
.equ    MAC1 =  0x00                    ;\
.equ    MAC2 =  0x00                    ;  \
.equ    MAC3 =  0x00                    ;    \
.equ    MAC4 =  0x12                    ;    48 bit IEEE OUI (Organizationally Unique Identifier)
.equ    MAC5 =  0x34                    ;  /
.equ    MAC6 =  0x56                    ;/
;
; Some useful macros
;
.macro  ppRD                            ;Read PacketPage port at addr @0
        ldi     offsetL,low(@0)
        ldi     offsetH,high(@0)
        rcall   ReadPP
.endmacro
;
.macro  ppWR                            ;Write PacketPage port (addr,val)
        ldi     offsetL,low(@0)
        ldi     offsetH,high(@0)
        ldi     valueL,low(@1)
        ldi     valueH,high(@1)
        rcall   WritePP
.endmacro

;
.macro  Dinput
        clr     wL                      ;Change the data bus to input
        out     DDRD,wL
.endmacro
;
.macro  Doutput        
        ser     wL                      ;Change the data bus to output
        out     DDRD,wL
.endmacro
;
.macro  ior                             ;I/O port read at @0 
        ldi     wL,@0
        rcall   ioRead
.endmacro
;
.macro  iow                             ;I/O port write at @0
        out     CDB,wL
        ldi     wL,@0
        out     CAB,wL
        rcall   ioWrite
.endmacro
;
; Crystal CS8900 PacketPage equates
;
.equ portRxTxData    =  0x00            ;Receive/Transmit data (port 0)
.equ portRxTxData1   =  0x02            ;Receive/Transmit data (port 0)
.equ portTxCmd       =  0x04            ;Transmit Commnad
.equ portTxLength    =  0x06            ;Transmit Length
.equ portISQ         =  0x08            ;Interrupt status queue
.equ portPtr         =  0x0a            ;PacketPage pointer
.equ portData        =  0x0c            ;PacketPage data (port 0)
.equ portData1       =  0x0e            ;PacketPage data (port 1)
;
; CS8900 PacketPage Offsets 
;
.equ ppEISA          =  0x0000          ;EISA Registration number of CS8900
.equ ppProdID        =  0x0002          ;Product ID Number
.equ ppIOBase        =  0x0020          ;I/O Base Address
.equ ppIntNum        =  0x0022          ;Interrupt number (0,1,2, or 3)
.equ ppMemBase       =  0x002C          ;Memory Base address register (20 bit)
.equ ppRxCfg         =  0x0102          ;Receiver Configuration
.equ ppRxCtl         =  0x0104          ;Receiver Control
.equ ppTxCfg         =  0x0106          ;Transmit Configuration
.equ ppBufCfg        =  0x010A          ;Buffer Configuration
.equ ppLineCtl       =  0x0112          ;Line Control
.equ ppSelfCtl       =  0x0114          ;Self Control
.equ ppBusCtl        =  0x0116          ;Bus Control
.equ ppTestCtl       =  0x0118          ;Test Control
.equ ppISQ           =  0x0120          ;Interrupt status queue
.equ ppRxEvt         =  0x0124          ;Receiver Event
.equ ppTxEvt         =  0x0128          ;Transmitter Event
.equ ppBufEvt        =  0x012C          ;Buffer Event
.equ ppRxMiss        =  0x0130          ;Receiver Miss Counter
.equ ppTxCol         =  0x0132          ;Transmit Collision Counter
.equ ppLineSt        =  0x0134          ;Line Status
.equ ppSelfSt        =  0x0136          ;Self Status
.equ ppBusSt         =  0x0138          ;Bus Status
.equ ppTxCmd         =  0x0144          ;Transmit Command Request
.equ ppTxLength      =  0x0146          ;Transmit Length
.equ ppIndAddr       =  0x0158          ;Individual Address (IA)
.equ ppRxStat        =  0x0400          ;Receive Status
.equ ppRxLength      =  0x0402          ;Receive Length
.equ ppRxFrame       =  0x0404          ;Receive Frame Location
.equ ppTxFrame       =  0x0A00          ;Transmit Frame Location
;
; Register Numbers 
;
.equ REG_NUM_MASK       =    0x003F
.equ REG_NUM_RX_EVENT   =    0x0004
.equ REG_NUM_TX_EVENT   =    0x0008
.equ REG_NUM_BUF_EVENT  =    0x000C
.equ REG_NUM_RX_MISS    =    0x0010
.equ REG_NUM_TX_COL     =    0x0012
;
; Self Control Register
;
.equ SELF_CTL_RESET     =    0x0040
.equ SELF_CTL_HC1E      =    0x2000
.equ SELF_CTL_HCB1      =    0x8000
;
; Self Status Register 
;
.equ SELF_ST_INIT_DONE  =    0x0080
.equ SELF_ST_SI_BUSY    =    0x0100
.equ SELF_ST_EEP_PRES   =    0x0200
.equ SELF_ST_EEP_OK     =    0x0400
.equ SELF_ST_EL_PRES    =    0x0800
;
; Bus Control Register 
;
.equ BUS_CTL_USE_SA     =    0x0200
.equ BUS_CTL_MEM_MODE   =    0x0400
.equ BUS_CTL_IOCHRDY    =    0x1000
.equ BUS_CTL_INT_ENBL   =    0x8000
;
; Bus Status Register 
;
.equ BUS_ST_TX_BID_ERR  =    0x0080
.equ BUS_ST_RDY4TXNOW   =    0x0100
;
; Line Control Register 
;
.equ LINE_CTL_RX_ON     =    0x0040
.equ LINE_CTL_TX_ON     =    0x0080
.equ LINE_CTL_AUI_ONLY  =    0x0100
.equ LINE_CTL_10BASET   =    0x0000
;
; Test Control Register 
;
.equ TEST_CTL_DIS_LT    =    0x0080
.equ TEST_CTL_ENDEC_LP  =    0x0200
.equ TEST_CTL_AUI_LOOP  =    0x0400
.equ TEST_CTL_DIS_BKOFF =    0x0800
.equ TEST_CTL_FDX       =    0x4000
;
; Receiver Configuration Register 
;
.equ RX_CFG_SKIP        =    0x0040
.equ RX_CFG_RX_OK_IE    =    0x0100
.equ RX_CFG_CRC_ERR_IE  =    0x1000
.equ RX_CFG_RUNT_IE     =    0x2000
.equ RX_CFG_X_DATA_IE   =    0x4000
;
; Receiver Event Register 
;
.equ RX_EVENT_RX_OK     =    0x0100
.equ RX_EVENT_IND_ADDR  =    0x0400
.equ RX_EVENT_BCAST     =    0x0800
.equ RX_EVENT_CRC_ERR   =    0x1000
.equ RX_EVENT_RUNT      =    0x2000
.equ RX_EVENT_X_DATA    =    0x4000
;
;Receiver Control Register 
;
.equ RX_CTL_RX_OK_A     =    0x0100
.equ RX_CTL_MCAST_A     =    0x0200
.equ RX_CTL_IND_A       =    0x0400
.equ RX_CTL_BCAST_A     =    0x0800
.equ RX_CTL_CRC_ERR_A   =    0x1000
.equ RX_CTL_RUNT_A      =    0x2000
.equ RX_CTL_X_DATA_A    =    0x4000
;
;Transmit Configuration Register 
;
.equ TX_CFG_LOSS_CRS_IE =    0x0040
.equ TX_CFG_SQE_ERR_IE  =    0x0080
.equ TX_CFG_TX_OK_IE    =    0x0100
.equ TX_CFG_OUT_WIN_IE  =    0x0200
.equ TX_CFG_JABBER_IE   =    0x0400
.equ TX_CFG_16_COLL_IE  =    0x8000
.equ TX_CFG_ALL_IE      =    0x8FC0
;
;Transmit Event Register 
;
.equ TX_EVENT_TX_OK     =    0x0100
.equ TX_EVENT_OUT_WIN   =    0x0200
.equ TX_EVENT_JABBER    =    0x0400
.equ TX_EVENT_16_COLL   =    0x1000
;
; Transmit Command Register
;
.equ TX_CMD_START_5     =    0x0000
.equ TX_CMD_START_381   =    0x0080
.equ TX_CMD_START_1021  =    0x0040
.equ TX_CMD_START_ALL   =    0x00C0
.equ TX_CMD_FORCE       =    0x0100
.equ TX_CMD_ONE_COLL    =    0x0200
.equ TX_CMD_NO_CRC      =    0x1000
.equ TX_CMD_NO_PAD      =    0x2000
;
;Buffer Configuration Register 
;
.equ BUF_CFG_SW_INT     =    0x0040
.equ BUF_CFG_RDY4TX_IE  =    0x0100
.equ BUF_CFG_TX_UNDR_IE =    0x0200
;
;
; Register file assignments
;
.def count=r14
.def lcdPos=r15
.def wL=r16
.def wH=r17
.def offsetL=r18
.def offsetH=r19
.def valueL=r20
.def valueH=r21
.def tempL=r22
.def tempH=r23
.def flags=r24
.def scratch=r25
.def lengthL=r26
.def lengthH=r27
.def chksumL=r0
.def chksumM=r1
.def chksumH=r2

;
;  start of packet data in sram
;
.equ packet = 0x60
;
; Packet offset equates
;
;
; Packet header
;
.equ pktLenH  =  0x00
.equ pktLenL  =  0x01
.equ pktDest0H=  0x02
.equ pktDest0L=  0x03
.equ pktDest1H=  0x04
.equ pktDest1L=  0x05
.equ pktDest2H=  0x06
.equ pktDest2L=  0x07
.equ pktSrc0H =  0x08
.equ pktSrc0L =  0x09
.equ pktSrc1H =  0x0a
.equ pktSrc1L =  0x0b
.equ pktSrc2H =  0x0c
.equ pktSrc2L =  0x0d
.equ pktTypeH =  0x0e
.equ pktTypeL =  0x0f
;
; ARP
;
.equ ar_hwtype=  0x10                   ;hardware type
.equ ar_prtype=  0x12                   ;protocol type
.equ ar_hwlen =  0x14                   ;hardware address length
.equ ar_prlen =  0x15                   ;protocol address length
.equ ar_op    =  0x16                   ;ARP operation (1=request, 2=reply)
.equ ar_sha   =  0x18                   ;senders hardware address
.equ ar_spa   =  0x1e                   ;senders IP address
.equ ar_tha   =  0x22                   ;target hardware address
.equ ar_tpa   =  0x28                   ;target IP address
;
; IP header
;
.equ ip_verlen=  0x10                   ;IP version and header length(in longs)
.equ ip_tos   =  0x11                   ;IP type of service
.equ ip_len   =  0x12                   ;packet length (length-header_length)
.equ ip_id    =  0x14                   ;datagram id
.equ ip_fragoff= 0x16                   ;fragment offset
.equ ip_ttl   =  0x18                   ;time to live (in gateway hops)
.equ ip_proto =  0x19                   ;protocol (ICMP=1, TCP=6, EGP=8, UDP=17)
.equ ip_cksum =  0x1a                   ;header checksum
.equ ip_src   =  0x1c                   ;IP address of source
.equ ip_dst   =  0x20                   ;IP addess of destination
.equ ip_data  =  0x24
;
; IP value equates
;
.equ IPT_ICMP =  1                      ;protocol type for ICMP packets
.equ IPT_TCP  =  6                      ;protocol type for TCP packets
.equ IPT_EGP  =  8                      ;protocol type for EGP packets
.equ IPT_UDP  =  0x11                   ;protocol type for UDP packets
;
; ICMP header
;
.equ ic_type  =  ip_data                ;0=reply, 8=request, others=who-cares
.equ ic_code  =  ic_type+1              ;code
.equ ic_cksum =  ic_code+1              ;checksum of header+data
.equ ic_id    =  ic_cksum+2             ;message id
.equ ic_seq   =  ic_id+2                ;sequence number
;
; UDP Header
;
.equ u_src    =  ip_data                ;source udp port number
.equ u_dst    =  u_src+2                ;destination UDP port number
.equ u_len    =  u_dst+2                ;length of UDP header+data
.equ u_cksum  =  u_len+2                ;checksum  (see note)
.equ u_data   =  u_cksum+2              ;start of data
;
; Note: checksum is calculated by taking the 16 bit sums of the ip_src, ip_dst,
; ip_proto, u_len, and the sum starting at u_src for a length of u_len
; yes, this means that u_len is taken twice! u_cksum is zero during the calc.
; The sum is then one's complemented. This is the checksum
;
;
start:
        ldi     wL,low(RAMEND)
        out     SPL,wL
        ldi     wL,high(RAMEND)		    ;setup the stack
        out     SPH,wL
		ldi     wL,$c0		            ;Setup for external
		out     MCUCR,wL		        ; SRAM Enable and Wait State Enable
        ldi     wL,0b11110111           ;PORTE INTR signal input all others output
        out     DDRE,wL                 
        sbi     CSIG,IOR                ;no read
        sbi     CSIG,IOW                ;no write
        sbi     CSIG,CHIPSEL            ;don't select chip
        ser     wL
        out     DDRB,wL                 ;PORTB (Crystal Address Bus) output
        clr     count                   ;packet counter
        Dinput

        ldi     scratch,255             ;wait
        rcall   wait
        
        rcall   lcd_reset               ;reset LCD

        rcall   VerChip                 ;make sure that the CS8900 is connected, Z set if true
        ldi     wL,'>'                  ;success indicator
        breq    VerOK
        ldi     wL,'<'                  ;failure indicator on LCD
VerOK:  rcall   lcd_write
        rcall   ResetChip               ;reset CS8900
        rcall   InitChip                ;initialize CS8900


;
; We already did this in VerChip but get the signature bytes for the CS8900 and display
; them on the LCD. Should be 3000 630E 0500
;
        Dinput
        ior     portPtr+1               ;read PacketPage Ptr for high order byte
        mov     valueH,wL               ;save high low
        ior     portPtr                 ;read PacketPage Ptr for high order byte
        mov     valueL,wL               ;save high low
        rcall   outHL

        ppRD    ppEISA                  ;get the EISA number whoch should be 0x630E
        rcall   outHL

        ppRD    ppProdID                ;get the Product ID which should be 000x xxxx 0000 0000
        rcall   outHL

        clr     flags                   ;Packet flags

loop:
        sbis    PINE,INTR               ;wait for interrupt
        rjmp    loop
        rcall   ProcessISQ              ;process the queue
        sbrs    flags,0                 ;received frame flag
        rjmp    loop
        cbr     flags,1                 ;clear the receive flag
;
; We're going to display on the LCD, t: CC LLLL FFFF
; Where t=r for recieved frame, a=arp request, p=ping request
;       CC=count of frames received so far
;       LLLL=length of this frame
;       FFFF=frame type. For example 0800 for IP, 0806 for ARP
;
        rcall   lcd_clear
        clr     lcdPos  
        inc     lcdPos                  ;start in second LCD position
        ldi     wL,':'
        rcall   lcd_write
		mov		wL,count
        rcall   toHexH
		rcall   lcd_write
        mov		wL,count                ;display the count of received frames
        rcall   toHexL
        rcall   lcd_write
        ldi     wL,' '
        rcall   lcd_write
        lds     valueH,packet+pktLenH   ;display the packet length
        lds     valueL,packet+pktLenL
        rcall   outHL
        lds     valueH,packet+pktTypeH  ;and the packet type
        lds     valueL,packet+pktTypeL
        rcall   outHL

        sbrc    flags,1                 ;bit set when valid arp
        rjmp    ArpRequest
        sbrc    flags,2                 ;bit set when valid UDP Request on port 7
        rjmp    UDP7Request
        sbrc    flags,3                 ;bit set when valid ICMP Ping request received
        rjmp    PingRequest
        clr     lcdPos
        ldi     wL,'r'
        rcall   lcd_write
        rjmp    loop

ArpRequest:
        cbr     flags,2                 ;clear the valid arp request flag
        clr     lcdPos
        ldi     wL,'a'
        rcall   lcd_write
        rcall   WriteARP                ;respond to the ARP request with our IP/MAC
        rjmp    loop
PingRequest:
        cbr     flags,8                 ;clear the valid ICMP ping request flag
        clr     lcdPos
        ldi     wL,'p'
        rcall   lcd_write
        rcall   WritePing               ;respond to the ping
        rjmp    loop
UDP7Request:
        cbr     flags,4                 ;clear the valid UDP port 7 request flag
        clr     lcdPos
        ldi     wL,'u'
        rcall   lcd_write
        rcall   WriteUDP                ;echo reply
        rjmp    loop


;
; Converts the valueH/valueL to hex and displays it on the LCD
;
outHL:
		mov		wL,valueH
        rcall   toHexH
		rcall	lcd_write
        mov		wL,valueH
        rcall   toHexL
        rcall   lcd_write
        mov		wL,valueL
        rcall   toHexH
		rcall	lcd_write
		mov		wL,valueL
        rcall   toHexL
        rcall   lcd_write
        ldi     wL,' '
        rcall   lcd_write
        ret
toHexL:
		andi	wL,0x0f
		ori		wL,0x30
		cpi		wL,0x3a
		brlo	thlX
        ldi     wH,7
		add  	wL,wH
thlX:   ret
toHexH:
		lsr		wL
		lsr		wL
		lsr		wL
		lsr		wL
		ori		wL,0x30
		cpi		wL,0x3a
		brlo	thhX
        ldi     wH,7
		add	    wL,wH
thhX:   ret

;
; Send the reset sequence to the LCD's HD44780 controller. 
;
lcd_reset:
        ldi     scratch,20          ;wait 20 milliseconds
        rcall   wait
		ldi		wL,$30				;Spec said to send this init sequence
		sts		$8000,wL
        ldi     scratch,4
		rcall	wait    			;Wait 4ms
		ldi		wL,$30				;Spec said to send this init sequence
		sts		$8000,wL			;Send init sequence again
		rcall	delay100us			;Wait 100us
		ldi		wL,$30				;Spec said to send this init sequence
		sts		$8000,wL			;and one last time
		rcall	delay100us			;Wait 100us
		ldi		wL,$3F				;Display off
		sts		$8000,wL
        ldi     scratch,4
		rcall	wait    			;Wait 4ms
		ldi		wL,$0F				;Display on
		sts		$8000,wL
        ldi     scratch,4
		rcall	wait    			;Wait 4ms
		ldi		wL,$06				;Entry Mode Set
		sts		$8000,wL
        ldi     scratch,4
		rcall	wait    			;Wait 4ms
        rcall   lcd_clear           ;clear display
        clr     lcdPos              ;position of character on LCD
		ret
;
; Clear the LCD display
;
lcd_clear:
		ldi		wL,$01				;Clear Display
		sts		$8000,wL
        ldi     scratch,4
		rcall	wait    			;Wait 4ms
        clr     lcdPos              ;position of character on LCD
        ret

		
;
; Sends the character in r16 to the LCD. The register "lcdPos" is
; incremented to the next position. The register value is a modulous 8
; and the LCD is broken into two sections (as is typical for HD44780 based LCDs),
; position 0-7 on LCD1 and 0-7 on LCD2.
;
lcd_write:
		mov		r17,lcdPos	
		sbrs	r17,3
		rjmp 	lcd_1
        andi    r17,0x07
		ori		r17,0x40
        rjmp    lcd_2
lcd_1:	andi    r17,0x07
lcd_2:  ori		r17,0x80
		sts		$8000,r17
		rcall	delay40us
		sts		$C000,r16
		rcall	delay40us
		inc		lcdPos
		ret	
		
delay40us:
        push    wL
		ldi		wL,$40
loop3:	dec		wL		
		brne	loop3	
        pop     wL	
		ret

delay100us:
        push    wL
		ldi		wL,$88
loop4:	dec		wL		
		brne	loop4
        pop     wL		
		ret

        
;
; wait for W milliseconds
;
;delay the given number of milliseconds using a software loop
;	Enter:	scratch = number of milliseconds to delay
;	Exit:	scratch,wL,wH = undefined
wait:
wait1:  ldi	    wH,high((ClkFreq / 1000) / 4)
	    ldi     wL,low((ClkFreq / 1000) / 4)
wait2:	subi	wL,1			;(1)
	    sbci	wH,0			;(1)
	    brne	wait2			;(2)
	    dec	    scratch
	    brne	wait1
	    ret

;************************************************************************
;*         C S 8 9 0 0   D r i v e r   F u n c t i o n s                *
;************************************************************************
;
; WriteARP - Send a response ARP to the requestor 
;
WriteARP:
        ldi     zL,low(packet)
        ldi     zH,high(packet)         ;load the z reg with the packet address
        clr     wL
        st      z,wL
        ldi     wL,42  				    ;length of ARP response frame
        std     z+2,wL

; Target HW address MAC (6)
        lds     wL,packet+pktSrc0H
        sts     packet+ar_tha,wL
        sts     packet+pktDest0H,wL
        lds     wL,packet+pktSrc0L
        sts     packet+ar_tha+1,wL
        sts     packet+pktDest0L,wL
        lds     wL,packet+pktSrc1H
        sts     packet+ar_tha+2,wL
        sts     packet+pktDest1H,wL
        lds     wL,packet+pktSrc1L
        sts     packet+ar_tha+3,wL
        sts     packet+pktDest1L,wL
        lds     wL,packet+pktSrc2H
        sts     packet+ar_tha+4,wL
        sts     packet+pktDest2H,wL
        lds     wL,packet+pktSrc2L
        sts     packet+ar_tha+5,wL
        sts     packet+pktDest2L,wL
; Target IP    (4)
        lds     wL,packet+ar_spa
        sts     packet+ar_tpa,wL
        lds     wL,packet+ar_spa+1
        sts     packet+ar_tpa+1,wL
        lds     wL,packet+ar_spa+2
        sts     packet+ar_tpa+2,wL
        lds     wL,packet+ar_spa+3
        sts     packet+ar_tpa+3,wL
; Senders IP    (4)
        ldi     wL,IP1
        sts     packet+ar_spa,wL
        ldi     wL,IP2
        sts     packet+ar_spa+1,wL
        ldi     wL,IP3
        sts     packet+ar_spa+2,wL
        ldi     wL,IP4
        sts     packet+ar_spa+3,wL

; Src HW Address MAC    (6)
        ldi     wL,MAC1
        sts     packet+ar_sha,wL
        sts     packet+pktSrc0H,wL
        ldi     wL,MAC2
        sts     packet+ar_sha+1,wL
        sts     packet+pktSrc0L,wL
        ldi     wL,MAC3    
        sts     packet+ar_sha+2,wL
        sts     packet+pktSrc1H,wL
        ldi     wL,MAC4
        sts     packet+ar_sha+3,wL
        sts     packet+pktSrc1L,wL
        ldi     wL,MAC5
        sts     packet+ar_sha+4,wL
        sts     packet+pktSrc2H,wL
        ldi     wL,MAC6
        sts     packet+ar_sha+5,wL
        sts     packet+pktSrc2L,wL
; Packet type    (2)
        ldi     wL,0x08
        sts     packet+pktTypeH,wL
        ldi     wL,0x06
        sts     packet+pktTypeL,wL
; Hardware type    (2)
        ldi     wL,0x00
        sts     packet+ar_hwtype,wL
        ldi     wL,0x01   
        sts     packet+ar_hwtype+1,wL
; Protocol type (2)
        ldi     wL,0x08
        sts     packet+ar_prtype,wL
        ldi     wL,0x00
        sts     packet+ar_prtype+1,wL
; Hardware and protocol address lengths (2)
        ldi     wL,0x06
        sts     packet+ar_hwlen,wL
        ldi     wL,0x04
        sts     packet+ar_prlen,wL
; Operation (response) (2)
        ldi     wL,0x00
        sts     packet+ar_op,wL
        ldi     wL,0x02
        sts     packet+ar_op+1,wL
        rcall   WritePacket
        ret

;
; WriteUDP - Send a response to the port 7 UDP echo request
;
WriteUDP:
        clr     wL
        sts     packet+ip_cksum,wL      ;clear
        sts     packet+ip_cksum+1,wL    ; checksum

        lds     wL,packet+ip_src        ;move
        sts     packet+ip_dst,wL        ; ip
        lds     wL,packet+ip_src+1      ;  of 
        sts     packet+ip_dst+1,wL      ;   source
        lds     wL,packet+ip_src+2      ;    to
        sts     packet+ip_dst+2,wL      ;     dest
        lds     wL,packet+ip_src+3 
        sts     packet+ip_dst+3,wL
   
        ldi     wL,IP1                  ;move
        sts     packet+ip_src,wL        ; our
        ldi     wL,IP2                  ;  ip
        sts     packet+ip_src+1,wL      ;   address
        ldi     wL,IP3                  ;    to
        sts     packet+ip_src+2,wL      ;     source
        ldi     wL,IP4   
        sts     packet+ip_src+3,wL

        lds     wL,packet+pktSrc0H      ;move
        sts     packet+pktDest0H,wL     ; mac src to dest
        lds     wL,packet+pktSrc0L      
        sts     packet+pktDest0L,wL       
        lds     wL,packet+pktSrc1H      ;move
        sts     packet+pktDest1H,wL     ; mac src to dest
        lds     wL,packet+pktSrc1L      
        sts     packet+pktDest1L,wL       
        lds     wL,packet+pktSrc2H      ;move
        sts     packet+pktDest2H,wL     ; mac src to dest
        lds     wL,packet+pktSrc2L      
        sts     packet+pktDest2L,wL

        ldi     wL,MAC1                 ;move
        sts     packet+pktSrc0H,wL      ; our
        ldi     wL,MAC2                 ;  MAC
        sts     packet+pktSrc0L,wL      ;   address
        ldi     wL,MAC3                 ;    to 
        sts     packet+pktSrc1H,wL      ;     source
        ldi     wL,MAC4
        sts     packet+pktSrc1L,wL
        ldi     wL,MAC5
        sts     packet+pktSrc2H,wL
        ldi     wL,MAC6
        sts     packet+pktSrc2L,wL

        ldi     lengthL,20              ;length of IP header
        clr     lengthH
        ldi     zL,low(packet+ip_verlen)
        ldi     zH,high(packet+ip_verlen)
        rcall   cksum
        sts     packet+ip_cksum,valueH  ;save new IP checksum
        sts     packet+ip_cksum+1,valueL

        lds     wH,packet+u_src         ;get src port
        lds     wL,packet+u_dst         ;put dest port 
        sts     packet+u_src,wL         ;in source
        sts     packet+u_dst,wH         ;put source port in dest

        lds     wH,packet+u_src+1       ;get src port
        lds     wL,packet+u_dst+1       ;put dest port 
        sts     packet+u_src+1,wL       ;in source
        sts     packet+u_dst+1,wH       ;put source port in dest

        clr     wL
        sts     packet+u_cksum,wL       ;clear UDP checksum for new calc
        sts     packet+u_cksum+1,wL     ;clear byte two

        ldi     zl,low(packet+ip_src)
        ldi     zh,high(packet+ip_src)
        lds     lengthH,packet+ip_len
        lds     lengthL,packet+ip_len+1
        ldi     wL,20-8                 ;ip_len-20+8 (20 is the length of an IP header.
        clr     wH                      ;  8 is the length of the two IP fields (ip_src/ip_dst)
        sub     lengthL,wL
        sbc     lengthH,wH
        rcall   cksum

        clr     wH
        lds     wL,packet+ip_proto      ;get protocol (1 byte)
        sub     valueL,wL               ;add to accum
        sbc     valueH,wH
        lds     wH,packet+u_len         ;add in UDP len
        lds     wL,packet+u_len+1
        sub     valueL,wL
        sbc     valueH,wH

        sts     packet+u_cksum,valueH
        sts     packet+u_cksum+1,valueL

        rcall   WritePacket

        ret


;
; WritePing - Send a ICMP Ping response
;
WritePing:
       ;we might want to check in the future for IA or broadcast requests
        clr     wL
        sts     packet+ip_cksum,wL
        sts     packet+ip_cksum+1,wL    ;clear checksum

        lds     wL,packet+ip_src        ;move
        sts     packet+ip_dst,wL        ; ip
        lds     wL,packet+ip_src+1      ;  of 
        sts     packet+ip_dst+1,wL      ;   source
        lds     wL,packet+ip_src+2      ;    to
        sts     packet+ip_dst+2,wL      ;     dest
        lds     wL,packet+ip_src+3 
        sts     packet+ip_dst+3,wL   
        ldi     wL,IP1                  ;move
        sts     packet+ip_src,wL        ; our
        ldi     wL,IP2                  ;  ip
        sts     packet+ip_src+1,wL      ;   address
        ldi     wL,IP3                  ;    to
        sts     packet+ip_src+2,wL      ;     source
        ldi     wL,IP4   
        sts     packet+ip_src+3,wL

        clr     wL
        sts     packet+ic_cksum,wL      ;clear ICMP
        sts     packet+ic_cksum+1,wL    ; checksum

        sts     packet+ic_type,wL       ;make type a reply
        sts     packet+ic_code,wL       ;clear code

        lds     wL,packet+pktSrc0H      ;move
        sts     packet+pktDest0H,wL     ; mac src to dest
        lds     wL,packet+pktSrc0L      
        sts     packet+pktDest0L,wL       
        lds     wL,packet+pktSrc1H      ;move
        sts     packet+pktDest1H,wL     ; mac src to dest
        lds     wL,packet+pktSrc1L      
        sts     packet+pktDest1L,wL       
        lds     wL,packet+pktSrc2H      ;move
        sts     packet+pktDest2H,wL     ; mac src to dest
        lds     wL,packet+pktSrc2L      
        sts     packet+pktDest2L,wL

        ldi     wL,MAC1                 ;move
        sts     packet+pktSrc0H,wL      ; our
        ldi     wL,MAC2                 ;  MAC
        sts     packet+pktSrc0L,wL      ;   address
        ldi     wL,MAC3                 ;    to 
        sts     packet+pktSrc1H,wL      ;     source
        ldi     wL,MAC4
        sts     packet+pktSrc1L,wL
        ldi     wL,MAC5
        sts     packet+pktSrc2H,wL
        ldi     wL,MAC6
        sts     packet+pktSrc2L,wL


        ldi     lengthL,20              ;set length of
        clr     lengthH                 ; checksum calculation =  length of IP header
        ldi     zL,low(packet+ip_verlen);displacement to start of IP header
        ldi     zH,high(packet+ip_verlen)
        rcall   cksum                   ;calculate the checksum

        sts     packet+ip_cksum,valueH  ;set new checksum
        sts     packet+ip_cksum+1,valueL

        lds     lengthH,packet+ip_len   ;get length high
        lds     lengthL,packet+ip_len+1 ;calc length of ICMP header+data
        subi    lengthL,20              ;-length of IP header
        sbci    lengthH,0
        ldi     zL,low(packet+ic_type)  ;start of ICMP header
        ldi     zH,high(packet+ic_type) ;start of ICMP header
        rcall   cksum

        sts     packet+ic_cksum,valueH  ;set new checksum
        sts     packet+ic_cksum+1,valueL

        rcall   WritePacket
ICMPDone:
        ret


;----------------------------------------------------------------------
; Receive Event
; Input with RxEvent in valueH
;
ReceiveEvent:
;
; It's important to read the status and length high-order
; byte first. 
;
        Dinput                          ;make data bus input
        ior     portRxTxData+1          ;read and discard status
        ior     portRxTxData            ;read and discard status
        ldi     yL,low(packet)          ;set indirect
        ldi     yH,high(packet)
        ior     portRxTxData+1          ;get length high
        st      y+,wL                   ;save in header
        mov     lengthH,wL              ; and in lengthH
        ior     portRxTxData            ;get length low
        st      y+,wL                   ;save in header area
        mov     lengthL,wL              ; and in lengthL
        sbrc    lengthL,0               ;odd length?
        adiw    lengthL,1               ;yes, increment lengthL/lengthH by 1
        lsr     lengthH                 ;divide high by 2 and set carry appropriately
        ror     lengthL                 ;rotate carry into lower and divide by 2
ReadFrame:
        ior     portRxTxData            ;read from CS8900
        st      y+,wL                   ;write it to sram
        ior     portRxTxData+1          ;read from CS8900
        st      y+,wL                   ;write it to sram
        sbiw    lengthL,1               ;decrement lengthL/lengthH
        brpl    ReadFrame

        inc     count                   ;increment packet counter
        sbr     flags,1                 ;flag frame read

        lds     wL,packet+pktTypeH      ;check if packet type
        cpi     wL,0x08                 ; is 0x0806 (ARP)
        brne    ChkIP                    
        lds     wL,packet+pktTypeL
        cpi     wL,0x06
        breq    DoArp                    
ChkIP:
        lds     wL,packet+pktTypeH      ;check if packet type
        cpi     wL,0x08                 ; is 0x0806 (ARP)
        brne    REReturn                       
        lds     wL,packet+pktTypeL
        cpi     wL,0x00
        breq    DoIP                    
REReturn:
        ret

;
; It's an arp request, Could be a request or a response.
; ARP packets are small so the whole thing is always in
; the register file.
;
DoArp:
        lds     wL,packet+ar_hwtype+1   ;make sure it's 1
        cpi     wL,0x01
        brne    DoArpX

        lds     wL,packet+ar_prtype     ;compare to high
        cpi     wL,0x08                 ;make sure it's 0x0800
        brne    DoArpX

        lds     wL,packet+ar_prtype+1   ;compare to high
        cpi     wL,0x00                 ;make sure it's 0x0800
        brne    DoArpX

        lds     wL,packet+ar_hwlen      ;make sure it's 6 for hwlen
        cpi     wL,0x06                 ;compare to high
        brne    DoArpX

        lds     wL,packet+ar_prlen      ;make sure it's 4 for protocol length
        cpi     wL,0x04                 ;compare to low
        brne    DoArpX

        lds     wL,packet+ar_op+1       ;make sure it's 0x0001 for arp request
        cpi     wL,0x01                 ;compare to low
        brne    DoArpX

        lds     wL,packet+ar_tpa
        cpi     wL,IP1
        brne    DoArpX        
        lds     wL,packet+ar_tpa+1
        cpi     wL,IP2
        brne    DoArpX        
        lds     wL,packet+ar_tpa+2
        cpi     wL,IP3
        brne    DoArpX        
        lds     wL,packet+ar_tpa+3
        cpi     wL,IP4
        brne    DoArpX        

; If we got to here, then we have a valid ARP request for our IP address

        sbr     flags,2                 ;set ARP flag
DoArpX:
        ret

;
; Process IP packet for ICMP or UDP requests
;
DoIP:
        lds     wL,packet+ip_proto      ;compare to this packet's proto
        cpi     wL,IPT_ICMP             ; to ICMP?
        breq    DoICMP                  ;go process the ICMP packet
        cpi     wL,IPT_UDP              ;  to UDP?
        breq    DoUDP                   ;go process the UDP packet
        ret                             ;just return, we dont process this type

;
; Process UDP Port 7 echo requests
;
DoUDP:
        lds     wL,packet+u_dst+1       ;compare to UDP packet
        cpi     wL,7                    ;port we're listing for requests on
        brne    DoUDPDone
        sbr     flags,4                 ;we have a request for our port 7
DoUDPDone:
        ret

;
; Process ICMP (ping) requests
;
DoICMP:
        sbr     flags,8                 ;set flag to indicate ICMP ping request
        ret


;
; Calc checksum. Reads packet at offset zL/zH for
; lengthH/lengthL bytes and calculates the checksum
; in valueH/valueL.
;
cksum:  clr     chksumL                 ;we do the arithmetic
        clr     chksumM                 ; using a 24
        clr     chksumH                 ;  bit area
	    sbrs    lengthL,0		        ;odd length?
	    rjmp	cksumC
	    mov	    yL,zL
	    mov	    yH,zH
	    add	    yL,lengthL
	    adc	    yH,lengthH
    	clr	    wL
	    st	    y,wL		        	;clear byte after last
	    adiw	lengthL,1
cksumC: lsr	    lengthH
    	ror	    lengthL
cksuml: ld      wH,z+                   ;get high byte of 16-bit word
        ld      wL,z+
        add     chksumL,wL              ;add to accum
        brcc    noLcarry
        ldi     wL,1
        add     chksumM,wL
        brcc    noLcarry
        add     chksumH,wL
noLcarry:
        add     chksumM,wH              ;add in the high byte
        brcc    noHcarry
        inc     chksumH
noHcarry:
        subi    lengthL,1
        sbci    lengthH,0
        clr     wL
        cpi     lengthL,0
        cpc     lengthH,wL
        breq    CkDone
        brpl    cksuml    
CkDone: add     chksumL,chksumH         ;add in the third byte of 24 bit area
        brcc    CkDone1
        inc     chksumM
CkDone1:mov     valueL,chksumL
	    com	    valueL
        mov     valueH,chksumM
	    com	    valueH
        ret


;
; Transmit Event
;
TransmitEvent:
        ret
;
; Processes the pending interrupt requests from the Interrupt Status Queue
;
ProcessISQ:
        ; Read the ISQ
NextEvt:
        Dinput
        ior     portISQ                 ;read interrupt status queue
        mov     valueL,wL               ;save value low
        ior     portISQ+1               ;read interrupt status queue high
        mov     valueH,wL               ;save value high
        tst     valueL                  ;get val
        breq    EvtRet
        cpi     valueL,REG_NUM_RX_EVENT 
        breq    evtRecv
        cpi     valueL,REG_NUM_TX_EVENT 
        breq    evtTran                 ;Ingore BufEvent, RxMiss, and TxCol
        rjmp    NextEvt
evtRecv:
        rcall   ReceiveEvent
        rjmp    NextEvt
evtTran:
        rcall   TransmitEvent
        rjmp    NextEvt
EvtRet: ret


;
; Initializes the chip 
;
InitChip:
         ppWR   ppLineCtl,LINE_CTL_10BASET  ;set to 10BaseT
         ppWR   ppTestCtl,TEST_CTL_FDX      ;set to full duplex
         ppWR   ppRxCfg,RX_CFG_RX_OK_IE     ;enable RxOK interrupt
         ppWR   ppRxCtl,(RX_CTL_RX_OK_A|RX_CTL_IND_A|RX_CTL_BCAST_A)
         ppWR   ppTxCfg,TX_CFG_ALL_IE
;
; Important: The IA needs to be byte revered IA=aa:bb:cc:dd:ee:ff
;
         ppWR   ppIndAddr,(MAC2<<8|MAC1)    ;0xbbaa Write
         ppWR   ppIndAddr+2,(MAC4<<8|MAC3)  ;0xddcc  out
         ppWR   ppIndAddr+4,(MAC6<<8|MAC5)  ;0xffee   48 bit IA
         ppWR   ppIntNum,0x00               ;INT is on INTRQ0

         ppRD   ppBusCtl                    ;get Bus Control
         sbr    valueH,0x80                 ;enable irq
         rcall  WritePP


         ppRD   ppLineCtl                   ;get Line Control
         sbr    valueL,0xc0                 ;set SerRxOn and SerTxOn
         rcall  WritePP
         ret

;
; Resets the CS8900
;
ResetChip: 
        ppWR    ppSelfCtl,SELF_CTL_RESET ;issue a reset to the chip
ResetWait:
        ldi     scratch,1               ;wait for
        rcall   wait                    ;a millisecond
        ppRD    ppSelfCtl               ;get the Self Control status
        sbrc    valueL,6                ;see it bit 6 (RESET) was cleared
        rjmp    ResetWait               ;no, then still in reset, wait some more
        ppRD    ppSelfSt                ;get self status
        sbrs    valueL,7                ;bit 7 is INITD     
        rjmp    ResetWait               ; when set, initialization of the CS8900 is done
        ret
    
;
; Verifies that the CS8900 is attached and sets the STATUS,Z flag to
; indicate success or not set if failure.
;
VerChip:
        Dinput                          ;make the data bus input
        ; first, get the signature at portPtr which should be 0x3000
        ior     portPtr                 ;read PacketPage Ptr
        mov     valueL,wL               ;save value low
        ior     portPtr+1               ;read PacketPage Ptr for high order byte
        mov     valueH,wL               ;save high low
        tst     valueL                  ;should be 0 (i.e. 0x3000 low is 0x00)
        brne    VerBad
        cpi     valueH,0x30             ;high part of 0x3000
        brne    VerBad
        ppRD    ppEISA                  ;get the EISA number whoch should be 0x630E
        cpi     valueL,0x0e
        brne    VerBad
        cpi     valueH,0x63
        brne    VerBad
        ppRD    ppProdID                ;get the Product ID which should be 000x xxxx 0000 0000
                                        ;where x xxxx = 0 0011 for rev E and 0 0101 for rev F
        tst     valueL                  ;set return status
VerBad: ret                             ;return with Z indicating success or not.

;
; Writes the packet for lengthL/lengthH
;
WritePacket:
        Doutput
        ldi     wL,TX_CMD_START_ALL
        iow     portTxCmd
        clr     wL
        iow     portTxCmd+1
        lds     wL,packet+pktLenL       ;length 
        iow     portTxLength            ; of packet 
        lds     wL,packet+pktLenH       ;  to send
        iow     portTxLength+1
WPGetStat:
        ppRD    ppBusSt                 ;get BusStatus
        sbrs    valueH,0                ;is BUS_ST_RDY4TXNOW (ready for transmit)
        rjmp    WPGetStat               ;no, wait for it

        Doutput
        ldi     zL,low(packet+pktDest0H)
        ldi     zH,high(packet+pktDest0H)
        lds     lengthH,packet+pktLenH
        lds     lengthL,packet+pktLenL
        sbrs    lengthL,0               ;odd length?
        rjmp    PacketEven              ;nope, continue
        adiw    lengthL,1               ;increment length
PacketEven:
        lsr     lengthH                 ;divide high by 2 and set carry appropriately
        ror     lengthL                 ;rotate carry into lower and divide by 2
PacketWriteLoop:
        ld      wL,z+                   ;get next byte
        iow     portRxTxData
        ld      wL,z+                   ;get next byte
        iow     portRxTxData+1
        sbiw    lengthL,1               ;decrement lengthL/lengthH
        brpl    PacketWriteLoop         
        ret


;
; Writes the value at valueH/valueL to the PagePacket register who's
; address is in offsetH/offsetL
;
WritePP:
        Doutput                         ;make the data bus output
        mov     wL,offsetL              ;get the low order byte
        iow     portPtr                 ;write to PacketPage Ptr
        mov     wL,offsetH              ;get the high order byte
        iow     portPtr+1               ;write to PacketPage ptr
        mov     wL,valueL               ;get low order value to write
        iow     portData                ;write to PacketPage data
        mov     wL,valueH               ;get high order data
        iow     portData+1              ;write to PacketPage data
        ret
;
; Reads the PagePacket at offsetH/offsetL and returns the result in
; valueH/valueL.
;
ReadPP:
        Doutput                         ;make the data bus output
        mov     wL,offsetL              ;get the offset to read
        iow     portPtr                 ;write to PacketPage Ptr
        mov     wL,offsetH              ;get the high order offset
        iow     portPtr+1               ;PagePacket Pointer Register high byte
        Dinput                          ;make the data bus input
        ior     portData                ;read the PacketPage data
        mov     valueL,wL               ;save value low
        ior     portData+1              ;read the high order byte
        mov     valueH,wL               ;save the high order byte value
        ret

ioRead:
        out     CAB,wL
        cbi     CSIG,CHIPSEL
        cbi     CSIG,IOR    
        nop
        nop
        in      wL,PIND      
        sbi     CSIG,IOR    
        sbi     CSIG,CHIPSEL
        ret
;
ioWrite:
        cbi     CSIG,CHIPSEL
        cbi     CSIG,IOW      
        nop
        sbi     CSIG,IOW       
        sbi     CSIG,CHIPSEL  
        ret


