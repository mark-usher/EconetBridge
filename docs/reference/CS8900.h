typedef unsigned char BYTE;
typedef unsigned int  WORD;
typedef unsigned long DWORD;
typedef unsigned int  BOOL;
#define TRUE  1
#define FALSE 0

typedef struct sEthernetIO {
  WORD RxTxData;   // Receive / Transmit Data Port
  WORD dummy1;     // not used in a 16 bit bus system
  WORD TxCMD;      // Transmit Command
  WORD TxLength;   // Transmit Length
  WORD ISQ;        // Interrupt Status Queue
  WORD PPP;        // Packet Page Pointer
  WORD PPD;        // Packet Page Data
  WORD dummy2;     // not used in a 16 bit bus system
} EthernetIO;

// IO Interface Definitions
#define cAutoIncrement              0x8000
#define cProductIdentificationCode  0x0000
#define cIOBaseAddress              0x0020
#define cMemBaseAddress             0x002C
#define cSelfCTL                    0x0114
#define cBusCTL                     0x0116
#define cSelfST                     0x0136

typedef struct sEthernetMem {        // Packet Page Address
// Bus Interface Registers
  DWORD ProductIdentificationCode;   // 0x0000
  BYTE  reserved1[28];               // 0x0004
  WORD  IOBaseAddress;               // 0x0020
  WORD  InterruptNumber;             // 0x0022
  WORD  DMAChannelNumber;            // 0x0024
  WORD  DMAStartOfFrame;             // 0x0026
  WORD  DMAFrameCount;               // 0x0028
  WORD  RxDMAByteCount;              // 0x002A
  DWORD MemBaseAddress;              // 0x002C
  DWORD BootPROMBaseAddress;         // 0x0030
  DWORD BootPROMAddressMask;         // 0x0034
  BYTE  reserved2[8];                // 0x0038
  WORD  EEPROMCommand;               // 0x0040
  WORD  EEPROMData;                  // 0x0042
  BYTE  reserved3[12];               // 0x0044
  WORD  ReceivedFrameByteCounter;    // 0x0050
  BYTE  reserved4[174];              // 0x0052
// Status and Contol Register
//   Configuration and Control Register
  WORD  reserved5;                   // 0x0100
  WORD  RxCFG;                       // 0x0102
  WORD  RxCTL;                       // 0x0104
  WORD  TxCFG;                       // 0x0106
  WORD  TxCMD;                       // 0x0108
  WORD  BufCFG;                      // 0x010A
  WORD  reserved6[3];                // 0x010C
  WORD  LineCTL;                     // 0x0112
  WORD  SelfCTL;                     // 0x0114
  WORD  BusCTL;                      // 0x0116
  WORD  TestCTL;                     // 0x0118
  WORD  reserved7[3];                // 0x011A
//   Status and Event Register
  WORD  ISQ;                         // 0x0120
  WORD  reserved8;                   // 0x0122
  WORD  RxEVENT;                     // 0x0124
  WORD  reserved9;                   // 0x0126
  WORD  TxEVENT;                     // 0x0128
  WORD  reserved10;                  // 0x012A
  WORD  BufEvent;                    // 0x012C
  WORD  reserved11;                  // 0x012E
  WORD  RxMISS;                      // 0x0130
  WORD  TxCOL;                       // 0x0132
  WORD  LineST;                      // 0x0134
  WORD  SelfST;                      // 0x0136
  WORD  BusST;                       // 0x0138
  WORD  reserved12;                  // 0x013A
  WORD  TDR;                         // 0x013C
  WORD  reserved13;                  // 0x013E
// Initiate Transmit Register
  WORD  reserved14[2];               // 0x0140
  WORD  TxCMD2;                      // 0x0144
  WORD  TxLength;                    // 0x0146
  WORD  reserved15[4];               // 0x0148
// Address Filter Register
  BYTE  LogicalAddressFilter[8];     // 0x0150
  BYTE  IndividualAddress[6];        // 0x0158
  BYTE  reserved16[418];             // 0x015E
// IO Interface mapped to this address
  EthernetIO IO;                     // 0x0300
  BYTE  reserved17[240];             // 0x0310
// Frame Location
  WORD  RXStatus;                    // 0x0400
  WORD  RXLength;                    // 0x0402
  BYTE  ReceiveFrameLocation[1532];  // 0x0404
  BYTE  TransmitFrameLocation[1536]; // 0x0A00
} EthernetMem;

extern EthernetMem volatile huge Ether;

// Register Numbers 
//#define  REG_NUM_MASK         0x003F
//#define  REG_NUM_RX_EVENT     0x0004
//#define  REG_NUM_TX_EVENT     0x0008
//#define  REG_NUM_BUF_EVENT    0x000C
//#define  REG_NUM_RX_MISS      0x0010
//#define  REG_NUM_TX_COL       0x0012

// Self Control Register
#define  SELF_CTL_RESET       0x0040
#define  SELF_CTL_HC1E        0x2000
#define  SELF_CTL_HCB1        0x8000

// Self Status Register 
#define  SELF_ST_INIT_DONE    0x0080
#define  SELF_ST_SI_BUSY      0x0100
#define  SELF_ST_EEP_PRES     0x0200
#define  SELF_ST_EEP_OK       0x0400
#define  SELF_ST_EL_PRES      0x0800

// Bus Control Register 

#define  BUS_CTL_USE_SA       0x0200
#define  BUS_CTL_MEM_MODE     0x0400
#define  BUS_CTL_IOCHRDY      0x1000
#define  BUS_CTL_INT_ENBL     0x8000

// Bus Status Register 
#define  BUS_ST_TX_BID_ERR    0x0080
#define  BUS_ST_RDY4TXNOW     0x0100

// Line Control Register 
#define  LINE_CTL_RX_ON       0x0040
#define  LINE_CTL_TX_ON       0x0080
#define  LINE_CTL_AUI_ONLY    0x0100
#define  LINE_CTL_10BASET     0x0000

// Test Control Register 

#define  TEST_CTL_DIS_LT      0x0080
#define  TEST_CTL_ENDEC_LP    0x0200
#define  TEST_CTL_AUI_LOOP    0x0400
#define  TEST_CTL_DIS_BKOFF   0x0800
#define  TEST_CTL_FDX         0x4000

// Receiver Configuration Register 
#define  RX_CFG_SKIP          0x0040
#define  RX_CFG_RX_OK_IE      0x0100
#define  RX_CFG_CRC_ERR_IE    0x1000
#define  RX_CFG_RUNT_IE       0x2000
#define  RX_CFG_X_DATA_IE     0x4000

// Receiver Event Register 
#define  RX_EVENT_RX_OK       0x0100
#define  RX_EVENT_IND_ADDR    0x0400
#define  RX_EVENT_BCAST       0x0800
#define  RX_EVENT_CRC_ERR     0x1000
#define  RX_EVENT_RUNT        0x2000
#define  RX_EVENT_X_DATA      0x4000

// Receiver Control Register 
#define  RX_CTL_RX_OK_A       0x0100
#define  RX_CTL_MCAST_A       0x0200
#define  RX_CTL_IND_A         0x0400
#define  RX_CTL_BCAST_A       0x0800
#define  RX_CTL_CRC_ERR_A     0x1000
#define  RX_CTL_RUNT_A        0x2000
#define  RX_CTL_X_DATA_A      0x4000

// Transmit Configuration Register 
#define  TX_CFG_LOSS_CRS_IE   0x0040
#define  TX_CFG_SQE_ERR_IE    0x0080
#define  TX_CFG_TX_OK_IE      0x0100
#define  TX_CFG_OUT_WIN_IE    0x0200
#define  TX_CFG_JABBER_IE     0x0400
#define  TX_CFG_16_COLL_IE    0x8000
#define  TX_CFG_ALL_IE        0x8FC0

// Transmit Event Register 
#define  TX_EVENT_TX_OK       0x0100
#define  TX_EVENT_OUT_WIN     0x0200
#define  TX_EVENT_JABBER      0x0400
#define  TX_EVENT_16_COLL     0x1000

// Transmit Command Register
#define  TX_CMD_START_5       0x0000
#define  TX_CMD_START_381     0x0080
#define  TX_CMD_START_1021    0x0040
#define  TX_CMD_START_ALL     0x00C0
#define  TX_CMD_FORCE         0x0100
#define  TX_CMD_ONE_COLL      0x0200
#define  TX_CMD_NO_CRC        0x1000
#define  TX_CMD_NO_PAD        0x2000

// Buffer Configuration Register 
#define  BUF_CFG_SW_INT       0x0040
#define  BUF_CFG_RDY4TX_IE    0x0100
#define  BUF_CFG_TX_UNDR_IE   0x0200


// Packet offset equates

// Packet header
#define  pktLenH     0x00
#define  pktLenL     0x01
#define  pktDest0H   0x02
#define  pktDest0L   0x03
#define  pktDest1H   0x04
#define  pktDest1L   0x05
#define  pktDest2H   0x06
#define  pktDest2L   0x07
#define  pktSrc0H    0x08
#define  pktSrc0L    0x09
#define  pktSrc1H    0x0a
#define  pktSrc1L    0x0b
#define  pktSrc2H    0x0c
#define  pktSrc2L    0x0d
#define  pktTypeH    0x0e
#define  pktTypeL    0x0f

// ARP
#define  ar_hwtype   0x10   // hardware type
#define  ar_prtype   0x12   // protocol type
#define  ar_hwlen    0x14   // hardware address length
#define  ar_prlen    0x15   // protocol address length
#define  ar_op       0x16   // ARP operation (1=request, 2=reply)
#define  ar_sha      0x18   // senders hardware address
#define  ar_spa      0x1e   // senders IP address
#define  ar_tha      0x22   // target hardware address
#define  ar_tpa      0x28   // target IP address

// IP header
#define  ip_verlen   0x10   // IP version and header length(in longs)
#define  ip_tos      0x11   // IP type of service
#define  ip_len      0x12   // packet length (length-header_length)
#define  ip_id       0x14   // datagram id
#define  ip_fragoff  0x16   // fragment offset
#define  ip_ttl      0x18   // time to live (in gateway hops)
#define  ip_proto    0x19   // protocol (ICMP=1, TCP=6, EGP=8, UDP=17)
#define  ip_cksum    0x1a   // header checksum
#define  ip_src      0x1c   // IP address of source
#define  ip_dst      0x20   // IP addess of destination
#define  ip_data     0x24

#if 0
struct IpHeader {
#if defined(LITTLE_ENDIAN_BITFIELD)
	__u8	ihl:4,
		version:4;
#elif defined (BIG_ENDIAN_BITFIELD)
	__u8	version:4,
  		ihl:4;
#else
#error	"Please fix <asm/byteorder.h>"
#endif
	__u8	tos;
	__u16	tot_len;
	__u16	id;
	__u16	frag_off;
	__u8	ttl;
	__u8	protocol;
	__u16	check;
	__u32	saddr;
	__u32	daddr;
	/*The options start here. */
};
#endif

// IP value equates
#define  IPT_ICMP    1      // protocol type for ICMP packets
#define  IPT_TCP     6      // protocol type for TCP packets
#define  IPT_EGP     8      // protocol type for EGP packets
#define  IPT_UDP     0x11   // protocol type for UDP packets

// ICMP header
#define  ic_type     ip_data     //  0=reply, 8=request, others=who-cares
#define  ic_code     ic_type+1   //  code
#define  ic_cksum    ic_code+1   //  checksum of header+data
#define  ic_id       ic_cksum+2  //  message id
#define  ic_seq      ic_id+2     //  sequence number

// UDP Header
#define  u_src       ip_data     // source udp port number
#define  u_dst       u_src+2     // destination UDP port number
#define  u_len       u_dst+2     // length of UDP header+data
#define  u_cksum     u_len+2     // checksum  (see note)
#define  u_data      u_cksum+2   // start of data

// Note: checksum is calculated by taking the 16 bit sums of the ip_src, ip_dst,
// ip_proto, u_len, and the sum starting at u_src for a length of u_len
// yes, this means that u_len is taken twice! u_cksum is zero during the calc.
// The sum is then one's complemented. This is the checksum



