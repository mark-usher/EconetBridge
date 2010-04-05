/**
 * \addtogroup apps
 * @{
 */

/**
 * \defgroup AUN
 * @{
 *
 * AUN Ethernet packet processing
 */

/**
 * \file
 *         Process Ethernet packets received
 * \author
 *         Mark Usher
 */


#include "aun.h"
#include "uip.h"
#include "uip_arp.h"
#include "serial.h"
#include "adlc.h"
#include "econet.h"
#include "nic.h"

#include <string.h>

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

//#define DEBUG 1
//#define DEBUGtx 1
#define BUF ((struct uip_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])

static struct router_state r_s;	// holds the connection to the stack. Defined aun_uip.h
volatile register unsigned char adlc_state asm ("r15");

uint8_t rTableEco[256];		// array to flag the routable Econet Networks
uint32_t rTableEthIP[256];	// array to hold the Ethernet IP a.b.c.x for routable networks
uint8_t rTableEthType[256];	//

static uint16_t fwd_timeout;

/*---- Prototypes ----------------------------------------------------------*/

static void do_uip_send (void);
static void new_aun_data_rx (void);
static void new_aun_wandata_rx (void);
static void process_msg (struct wan_packet *w, uint16_t pktlen);

/*******************************************************************
*
* NAME 	      :	do_uip_send          
*
* DESCRIPTION :	
*
* INPUTS      :	Nothing
*
* OUTPUTS     : Nothing
*
* NOTES       :  	
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/

static void
do_uip_send (void)
{

  uip_process (UIP_UDP_SEND_CONN);
//serial_tx('B');
//serial_packet(&uip_buf,60);
  uip_arp_out ();
//serial_tx('C');
//serial_packet(&uip_buf,60);

  nic_send (NULL);
  uip_len = 0;

}

/*******************************************************************
*
* NAME 	      :	not_listening          
*
* DESCRIPTION :	
*
* INPUTS      :	Nothing
*
* OUTPUTS     : Nothing
*
* NOTES       :  	
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/

static void
not_listening (void)
{
  adlc_forwarding_complete (NOT_LISTENING, NULL, 0);
  forwarding = 0;
}

/*******************************************************************
*
* NAME 	      :	aun_init           
*
* DESCRIPTION :	The initialization function. We must explicitly call 
* 		this function from the system initialization code, 
*		some time after uip_init() is called
*
* INPUTS      : Nothing
*
* OUTPUTS     : Nothing
*
* NOTES	      :   
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/

void
aun_init (void)
{
  rTableEco[LOCAL_NETWORK] = ROUTABLE;
  rTableEco[ANY_NETWORK] = ROUTABLE;
  rTableEco[eeprom.Econet_Network] = ROUTABLE;

  // set the ethernet routing table
  rTableEthIP[eeprom.Ethernet_Network] =
    (uint32_t) (0x0201 |
		(uint32_t) ((uint32_t) eeprom.Ethernet_Network << 16));
  rTableEthType[eeprom.Ethernet_Network] = ETH_TYPE_LOCAL;

  // remove any open connections
  if (r_s.conn != NULL)
    {
      uip_udp_remove (r_s.conn);
    }

  // listen on the AUN port from all IP addresses

  r_s.conn = uip_udp_new (NULL, HTONS (MNSDATAPORT));	// AUN Protocol
  r_s.conn->lport = HTONS (MNSDATAPORT);
  r_s.wanconn = uip_udp_new (NULL, HTONS (WANDATAPORT));	// WAN DATA
  r_s.wanconn->lport = HTONS (WANDATAPORT);

}

/*******************************************************************
*
* NAME 	      :	aun_poller           
*
* DESCRIPTION :	
*
* INPUTS      :	Nothing
*
* OUTPUTS     : Nothing
*
* NOTES       :	   
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/

void
aun_poller (void)
{

  if (forwarding)
    {
      if (--fwd_timeout == 0)
	{
	  serial_tx ('T');
	  not_listening ();
	}
    }

}

/*****************************************************************************
*
* NAME 	      :	aun_appcall          
*
* DESCRIPTION :	In AUN.h we have defined the UIP_APPCALL macro to
* 		AUN_appcall so that this funcion is uIP's application
* 		function. This function is called whenever an uIP event occurs
* 		(e.g. when a new connection is established, new data arrives, 
*		sent data is acknowledged, data needs to be retransmitted, etc)
*
*		This will currently redirect data according to the port it is
*		received on. TELNET, AUN, AUN WAN, AUN ATP
*
* INPUTS      :	Nothing
*
* OUTPUTS     : Nothing
*
* NOTES       :	   
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/

void
aun_appcall (void)
{

#if DEBUG
  serial_tx_str ("aun_appcall.port:");
  serial_shortLH (uip_udp_conn->lport);
  serial_tx_str (" ");
  serial_tx_hex (eeprom.Econet_Network);
  serial_tx_str ("\r\n");
#endif

  /*---- Telnet ------------------------------------------------------------*/

  if (uip_conn->lport == HTONS (TELENTPORT))
    {
      telnetd_appcall ();	// Handle TELNET packets
      return;
    }

  /*---- AUN Data ----------------------------------------------------------*/

  if (uip_udp_conn->lport == HTONS (MNSDATAPORT))
    {
      if (uip_newdata ())
	{
	  new_aun_data_rx ();	// Handle AUN Packets
	}
    }

  /*---- AUN WAN Data ------------------------------------------------------*/

  else if (uip_udp_conn->lport == HTONS (WANDATAPORT))
    {
      if (uip_newdata ())
	{
	  new_aun_wandata_rx ();	// Handle AUN WAN traffic
	}
    }

  /*---- AUN ATP Requests --------------------------------------------------*/

#if 0
  if (uip_udp_conn->rport == HTONS (MNSATPPORT))
    {
      if (uip_newdata ())
	{
	  newdata ();		// Handle AUN ATP requests
	}
    }
#endif
}

/*****************************************************************************
*
* NAME 	      :	new_aun_data_rx         
*
* DESCRIPTION :	New data received on the AUN port
*
* INPUTS      :	Nothing
*
* OUTPUTS     : Nothing
*
* NOTES       :  	
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/

static void
new_aun_data_rx (void)
{

#if DEBUG
  serial_tx_str ("new_aun_data_rx.");
#endif

  struct wan_packet *w = (struct wan_packet *) uip_appdata;
  w--;

  unsigned char DNet, DStn;
  unsigned char SNet, SStn;

  SNet = (unsigned char) *(uip_buf + 28);	// octet 3 of source IP
  SStn = (unsigned char) *(uip_buf + 29);	// octet 4 of source IP
  DNet = (unsigned char) *(uip_buf + 32);	// octet 3 of destin IP
  DStn = (unsigned char) *(uip_buf + 33);	// octet 4 of destin IP

  w->dstn = DStn;
  w->dnet = DNet;
  w->sstn = SStn;
  w->snet = SNet;

  process_msg (w, uip_len - AUNHDRSIZE);
}

/*****************************************************************************
*
* NAME 	      :	new_aun_wandata_rx          
*
* DESCRIPTION :	New data received on the AUN WAN port
*
* INPUTS      :	Nothing
*
* OUTPUTS     : Nothing
*
* NOTES       :  	
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/

static void
new_aun_wandata_rx (void)
{
  struct wan_packet *w = (struct wan_packet *) uip_appdata;

  switch (w->opcode)
    {
    case 0x00:
      process_msg (w, uip_len - AUNHDRSIZE - sizeof (struct wan_packet));
      break;
    }
}


/*****************************************************************************
*
* NAME 	      :	process_msg          
*
* DESCRIPTION :	Called when new UDP data arrives.
*
* INPUTS      :	Nothing
*
* OUTPUTS     : Nothing
*
* NOTES       :	Internal 
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/

static void
process_msg (struct wan_packet *w, uint16_t pktlen)
{

#if DEBUG
  serial_tx_str ("process_msg.");
#endif

  /* if the destination econet network is the same as the network
     definition on this network interface, change it to 0, the
     local ie. immediate, network */

  if (w->dnet == eeprom.Econet_Network)
    w->dnet = LOCAL_NETWORK;

  struct mns_msg *m = (struct mns_msg *) (w + 1);

#if DEBUG
  serial_tx_str ("op.");
  serial_tx_hex (m->mns_opcode);
  serial_tx_str ("\r\n\n");
#endif

  // Process the packet according to the AUN Opcode
  switch (m->mns_opcode)
    {
    case BROADCAST_DATA_FRAME:	//1
    case DATA_FRAME:		//2
    case IMMEDIATE_OP:		//5
      forward_aun_packet (w, pktlen, m->mns_opcode);	// All these forward the packet
      break;
    case DATA_FRAME_ACK:	//3             Ack Frame
      if (forwarding)
	{
	  adlc_forwarding_complete (TX_OK, NULL, 0);
	  forwarding = 0;
	}
      break;
    case DATA_FRAME_REJ:	//4             Frame rejected by target
      if (forwarding)
	{
	  not_listening ();
	}
      break;
    case IMMEDIATE_OP_REPLY:	//6
      foward_immediate_op_reply (w, pktlen, m->mns_control);	// All these forward the packet
      break;
    default:
      // unknown mns opcode in frame
      not_listening ();
      break;
    }

}

/*****************************************************************************
*
* NAME 	      :	forward_aun_packet         
*
* DESCRIPTION :	forwards an ethernet payload to Econet
*
* INPUTS      :	WAN packet, data payload length, MNS Opcode
*
* OUTPUTS     : Nothing
*
* NOTES       :  	
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/

void
forward_aun_packet (struct wan_packet *w, unsigned short pkt_len,
		    uint8_t type)
{
  struct mns_msg *ah;
  struct Econet_Header *eh;


  // use Econet header structure instead of scout packet structure
  // so the Port byte can easily be duplicated.
  ah = (struct mns_msg *) (uip_appdata);
  eh = (struct Econet_Header *) (uip_appdata + 2);	// start the return packet
							  // 2 bytes in

  // copy these now or they will be overwritten
  uint32_t handle = ah->mns_handle;
  uint8_t port = ah->mns_port;
  uint8_t cb = ah->mns_control;

  eh->DSTN = w->dstn;
  eh->DNET = w->dnet;
  eh->SSTN = w->sstn;
  eh->SNET = w->snet;

  if (type == IMMEDIATE_OP_REPLY)
  {
      eh->CB = 0xFF;
      eh->PORT = 0xFF;		// this won't be transmitted but forces the id for 
      			 	// tx as normal not immediate in do_tx_packet
  }
  else
      eh->CB = cb | 0x80;
      eh->PORT = port;

  struct mbuf *mb = copy_to_mbufs ((uint8_t *) eh, pkt_len + 6);

  // put the memory buffer in the transmit queue
  if ((type == BROADCAST_DATA_FRAME) || (type == IMMEDIATE_OP_REPLY))
    enqueue_tx (mb);				// add mbuf to a tx_record
  else
    enqueue_aun_tx (mb, BUF, handle, cb);	// add mbuf to a tx_record with aun
  // specifics set
}



/*****************************************************************************
*
* NAME 	      :	foward_immediate_op_reply          
*
* DESCRIPTION :	
*
* INPUTS      :	forward immediate op reply from aun -> econet
*
* OUTPUTS     : Nothing
*
* NOTES       :  	
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/

void
foward_immediate_op_reply (struct wan_packet *w, unsigned short pkt_len,
			   uint8_t immediate_op)
{
//  struct mns_msg *ah;
//  struct Econet_Header *eh;


  // use Econet header structure instead of scout packet structure
  // so the Port byte can easily be duplicated.
//  ah = (struct mns_msg *) (uip_appdata);
//  eh = (struct Econet_Header *) (uip_appdata + 2);

  switch (immediate_op && RXCB)	// holds the immediate op code
    {
    case ECONET_PEEK:		// 1 Copy memory from another machine
      forwarding = 0;
      forward_aun_packet (w, pkt_len, IMMEDIATE_OP_REPLY);
      adlc_state = RX_IDLE;
      adlc_poller();
      break;
    case ECONET_POKE:		// 2 Copy memory to another machine
    case ECONET_JSR:		// 3 Cause JSR/BL on another machine
    case ECONET_USER_RPC:	// 4 Execute User remote procedure call
    case ECONET_OS_RPC:		// 5 Execute OS remote procedure call
      adlc_forwarding_complete (TX_OK, uip_appdata + 8, 4);
      break;
    case ECONET_HALT:		// 6 Halt the remote machine
    case ECONET_CONTINUE:	// 7 Continue the remote machine
    case ECONET_MACHINETYPE:	// 8 Machine peek of another machine
      adlc_forwarding_complete (TX_OK, uip_appdata + 8, 4);
      break;
    case ECONET_GET_REGS:	// 9 Return registers from another machine  
      adlc_forwarding_complete (TX_OK, uip_appdata + 8, 4);
      break;
    default:
      not_listening ();
      break;
    }

}




/*****************************************************************************
*
* NAME 	      :	do_atp          
*
* DESCRIPTION :	
*
* INPUTS      :	Network, Station
*
* OUTPUTS     : Nothing
*
* NOTES       :  	
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/

void
do_atp (unsigned char *Net, unsigned char *Stn)
{
  struct atp_block *atp;

  atp = ((struct atp_block *) &uip_buf[UIP_LLH_LEN]);

  atp->atpb_net = *Net;
  atp->atpb_station = *Stn;
}

/*****************************************************************************
*
* NAME 	      :	aun_send_immediate          
*
* DESCRIPTION :	Forwards an immediate packet from Econet -> Ethernet
*		aun_send_immediate() works in the same way as 
*		aun_send_packet() but for immediate ops. 
*		You need to call back to adlc_immediate_complete(), 
*		again with either TX_OK or NOT_LISTENING.  If the 
*		result was TX_OK, and there was data returned 
*		(i.e. from machine peek) then you need to supply a 
*		buffer and length as well. 
*
* INPUTS      :	Scout packet, destination IP address, data length
*
* OUTPUTS     : Nothing
*
* NOTES       :  	
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/

void
aun_send_immediate (uint8_t cb,
		 uint8_t port,
		 struct scout_packet *s, uint16_t data_length)
{

  // set up an AUN header for the packet by defining the start of uip_appdata
  // as an AUN header structure
  struct mns_msg *ah;
  uip_appdata = &uip_buf[UIP_LLH_LEN + UIP_IPUDPH_LEN];
  ah = (struct mns_msg *) (uip_appdata);

  // fill in the details of the AUN header
  ah->mns_opcode = IMMEDIATE_OP;	// type data frame
  ah->mns_port = port;			// destination port
  ah->mns_control = cb & RXCB;		// control byte
  ah->mns_status = 0;			// status
  ah->mns_handle = ++ r_s.handle;	// set a new handle
 
  uint8_t src_net = s->SNet;		// source network

  // if the source network is LOCAL (0) change it to the bridge's configured
  // network.

  if (src_net == LOCAL_NETWORK)
      src_net = eeprom.Econet_Network;


  // set the source ip address translation
  BUF->srcipaddr[0] = uip_hostaddr[0];	// first half same as host
  BUF->srcipaddr[1] = (s->SStn << 8) | src_net;	// station . network rev order
  BUF->destipaddr[0] = uip_hostaddr[0];	// first half same as host
  BUF->destipaddr[1] = (s->DStn << 8) | s->DNet;	// first half same as host
  BUF->destport = BUF->srcport = HTONS (MNSDATAPORT);	// set destination port

  uip_udp_send (AUNHDRSIZE + data_length);

  forwarding = 1;		// set the flag for the aun_poller routine to wait for a reply
  fwd_timeout = 20000;	// set the timeout value

//serial_packet(uip_appdata,AUNHDRSIZE + data_length);

  // packet is ready to send,
  do_uip_send ();

}

/*****************************************************************************
*
* NAME 	      :	aun_send_packet          
*
* DESCRIPTION :	
*
* INPUTS      :	control byte, port, Source Station & Network
*		Destination IP, 
*		datalength (of data in uip_appdata without the AUN header)
*
* OUTPUTS     : Nothing
*
* NOTES       :  	
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/

void
aun_send_packet (uint8_t cb,
		 uint8_t port,
		 uint16_t src_stn_net,
		 uip_ipaddr_t dest_ip, uint16_t data_length)
{

  // set up an AUN header for the packet by defining the start of uip_appdata
  // as an AUN header structure
  struct mns_msg *ah;
  ah = (struct mns_msg *) (uip_appdata);

  // fill in the details of the AUN header

  ah->mns_opcode = DATA_FRAME;	// type data frame
  ah->mns_port = port;		// destination port
  ah->mns_control = cb & RXCB;	// control byte
  ah->mns_status = 0;		// status
  ah->mns_handle = ++r_s.handle;	// handle

  uint8_t src_stn = src_stn_net & 0xff;	// source station
  uint8_t src_net = src_stn_net >> 8;	// source network

  // if the source network is LOCAL (0) change it to the bridge's configured
  // network.

  if (src_net == LOCAL_NETWORK)
      src_net = eeprom.Econet_Network;

  // set the source ip address translation
  BUF->srcipaddr[0] = uip_hostaddr[0];	// first half same as host
  BUF->srcipaddr[1] = (src_stn << 8) | src_net;	// station . network rev order
  uip_ipaddr_copy (BUF->destipaddr, dest_ip);	// set destination ip
  BUF->destport = BUF->srcport = HTONS (MNSDATAPORT);	// set destination port

  uip_udp_send (AUNHDRSIZE + data_length);

  forwarding = 1;		// set the flag for the aun_poller routine to wait for a reply
  fwd_timeout = 20000;		// set the timeout value

  // packet is ready to send,
  do_uip_send ();


}

/*****************************************************************************
*
* NAME 	      :	aun_send_broadcast          
*
* DESCRIPTION :	For broadcast operations. There's no acknowledgement 
*		for these, you just need to send them off.
*
* INPUTS      :	Scout Packet, data length
*
* OUTPUTS     : Nothing
*
* NOTES       :  	
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/

void
aun_send_broadcast (struct scout_packet *s, uint16_t data_length)
{
}

/*****************************************************************************
*
* NAME 	      :	aun_tx_complete          
*
* DESCRIPTION :	
*
* INPUTS      :	Status, pointer to a Transmit Record
*
* OUTPUTS     : Nothing
*
* NOTES       :  	
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/

void
aun_tx_complete (int8_t status, struct tx_record *tx)
{

#define BUF ((struct uip_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])

uint8_t cb = tx->cb & RXCB;
uint16_t data_length = AUNHDRSIZE;

  struct mns_msg *ah;
  uip_appdata = &uip_buf[UIP_IPUDPH_LEN + UIP_LLH_LEN];
  ah = (struct mns_msg *) (uip_appdata);

  uip_ipaddr_copy (BUF->destipaddr, tx->requestor_ip);
  uip_ipaddr_copy (BUF->srcipaddr, tx->target_ip);

  memset (ah, 0, AUNHDRSIZE);

  if ((tx->r_mb) && (cb >= ECONET_PEEK) && (cb <= ECONET_GET_REGS)) // immediate op?
  {
    ah->mns_opcode = DATA_FRAME_ACK; // IMMEDIATE_OP_REPLY;
    ah->mns_control = cb;
  }
  else
  {
    ah->mns_opcode = (status == TX_OK) ? DATA_FRAME_ACK : DATA_FRAME_REJ;
  }

  ah->mns_handle = tx->requestor_handle;


  if (tx->r_mb)			// data to be added
  {
	data_length += copy_from_mbufs(&uip_appdata[AUNHDRSIZE], tx->r_mb);
  }

  uip_udp_send (data_length);	// Send a UDP datagram on the current connection.
  do_uip_send ();

}

/*****************************************************************************
*
* NAME 	      :	aun_want_proxy_arp          
*
* DESCRIPTION :	
*
* INPUTS      :	pointer to an IP address
*
* OUTPUTS     : 1, 0
*
* NOTES       :  	
*
* CHANGES :
* REF NO    DATE    WHO     DETAIL
*
*/
uint8_t
aun_want_proxy_arp (uint16_t * ipaddr)
{
  if (ipaddr[0] == uip_hostaddr[0] && rTableEco[ipaddr[1] & ROUTABLE])
    return 1;
  return 0;
}
