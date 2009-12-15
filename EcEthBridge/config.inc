; config.inc

; This file holds indivdual config info that is different between users
;

; source IP address
.equ	ip_S_Addr_A		= 0x1
.equ	ip_S_Addr_B		= 0x2
.equ	ip_S_Addr_C		= 0x80
.equ	ip_S_Addr_D		= 0x0A

.equ	ip_SubNet_A		= 0xFF
.equ	ip_SubNet_B		= 0xFF
.equ	ip_SubNet_C		= 0x0
.equ	ip_SubNet_D		= 0x0

.equ	ip_GW_A		= 0x1
.equ	ip_GW_B		= 0x2
.equ	ip_GW_C		= 0x80
.equ	ip_GW_D		= 0x09

; destination IP address
.equ	ip_D_Addr_A		= 0x1
.equ	ip_D_Addr_B		= 0x2
.equ	ip_D_Addr_C		= 0x80
.equ	ip_D_Addr_D		= 0x09


; destination MAC address
.equ	MAC_Dest_5		= 0x00 
.equ	MAC_Dest_4		= 0x01 
.equ	MAC_Dest_3		= 0x4A 
.equ	MAC_Dest_2		= 0x60 
.equ	MAC_Dest_1		= 0xAC 
.equ	MAC_Dest_0		= 0x63 

.equ	EconetFS		= 0x04
.equ	EconetStation	= 0x53
.equ	EconetNetA		= 0x00

.equ duplexMode		= 0x40
