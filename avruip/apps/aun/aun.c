/**
 * \addtogroup AUN
 * @{
 */

/**
 * \file
 *         An example of how to write uIP applications
 *         with protosockets.
 * \author
 *         Mark Usher
 */

/*
 * This is a short example of how to write uIP applications using
 * protosockets.
 */

/*
 * We define the application state (struct hello_world_state) in the
 * hello-world.h file, so we need to include it here. We also include
 * uip.h (since this cannot be included in hello-world.h) and
 * <string.h>, since we use the memcpy() function in the code.
 */
#include "aun.h"
#include "uip.h"
#include "serial.h"
#include "adlc.h"
#include "econet.h"
#include "nic.h"

#include <string.h>

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

static struct aun_state s;

#define BUF ((struct uip_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])

#define STATE_INITIAL         0
#define STATE_SENDING         1
#define STATE_OFFER_RECEIVED  2
#define STATE_CONFIG_RECEIVED 3



struct EconetRouting
{
u_char	pad[UIP_LLH_LEN];
u_char	Network;
u_char	Station;
};


struct Econet_Header {
	unsigned char DSTN;
	unsigned char DNET;
	unsigned char SSTN;
	unsigned char SNET;
	unsigned char CB;
	unsigned char PORT;
	unsigned char DATA1;
	unsigned char DATA2;
	unsigned char DATA3;
	unsigned char DATA4;
	unsigned char DATA5;
	unsigned char DATA6;
	unsigned char DATA7;
	unsigned char DATA8;
};


uint8_t rTableEco[256];
uint32_t rTableEthIP[256];
uint8_t rTableEthType[256];

static void newdata(void);
static void newwandata(void);

static char forwarding;
static uint16_t fwd_timeout;

static void do_uip_send(void)
{
  uip_process(UIP_UDP_SEND_CONN);
  uip_arp_out();
  nic_send(NULL);
  uip_len = 0;
}

static void not_listening(void)
{
  adlc_forwarding_complete (NOT_LISTENING, NULL, 0);
  forwarding = 0;
}

/*---------------------------------------------------------------------------*/
/*
 * The initialization function. We must explicitly call this function
 * from the system initialization code, some time after uip_init() is
 * called.
 */
void
aun_init(void)
{
	rTableEco[LOCAL_NETWORK] = ROUTABLE;
	rTableEco[ANY_NETWORK] = ROUTABLE;
	rTableEco[eeprom.Econet_Network] = ROUTABLE;

	// set the ethernet routing table
	rTableEthIP[eeprom.Ethernet_Network] = (uint32_t)(0x0201 | (uint32_t)((uint32_t)eeprom.Ethernet_Network << 16));
	rTableEthType[eeprom.Ethernet_Network] = ETH_TYPE_LOCAL;

	// remove any open connections
	if (s.conn != NULL) {
		uip_udp_remove(s.conn);
	}

	// listen on the AUN port from all IP addresses

	s.conn = uip_udp_new(NULL, HTONS(MNSDATAPORT));
	s.conn->lport = HTONS(MNSDATAPORT);
	s.wanconn = uip_udp_new(NULL, HTONS(WANDATAPORT));
	s.wanconn->lport = HTONS(WANDATAPORT);
}

void aun_poller(void)
{
  if (forwarding) {
    if (--fwd_timeout == 0) {
      serial_tx('T');
      not_listening();
    }
  }
}

/*---------------------------------------------------------------------------*/
/*
 * In AUN.h we have defined the UIP_APPCALL macro to
 * AUN_appcall so that this funcion is uIP's application
 * function. This function is called whenever an uIP event occurs
 * (e.g. when a new connection is established, new data arrives, sent
 * data is acknowledged, data needs to be retransmitted, etc.).
 */
void
aun_appcall(void)
{

// Handle TELNET packets

  if(uip_conn->lport == HTONS(23)) {
	telnetd_appcall();
	return;
  }

// Handle AUN Packets

  if(uip_udp_conn->lport == HTONS(MNSDATAPORT)) {
   if(uip_newdata())
   {
     newdata();
   }
  }
  else if (uip_udp_conn->lport == HTONS(WANDATAPORT)) {
   if(uip_newdata())
   {
     newwandata();
   }
  }

#if 0
// Handle AUN ATP requests

  if(uip_udp_conn->rport == HTONS(MNSATPPORT)) {
   if(uip_newdata())
   {
      newdata();
    }
  }
#endif
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/** \internal
 * Called when new UDP data arrives.
 */
/*---------------------------------------------------------------------------*/
static void
process_msg(struct wan_packet *w, uint16_t pktlen)
{
	if (w->dnet == eeprom.Econet_Network) {
		w->dnet = LOCAL_NETWORK;
	}

	struct mns_msg *m = (struct mns_msg *)(w + 1);

	switch (m->mns_opcode)
	{
	case BROADCAST_DATA_FRAME: 	//1
	case DATA_FRAME:		//2
	case IMMEDIATE_OP:		//5
		foward_packet(w, pktlen, m->mns_opcode);
		//
		break;
	case DATA_FRAME_ACK:		//3
	  if (forwarding) {
	    adlc_forwarding_complete (TX_OK, NULL, 0);
	    forwarding = 0;
	  }
		//
		break;
	case DATA_FRAME_REJ:		//4
	  if (forwarding) {
	    not_listening();
	  }
		//
		break;
	case IMMEDIATE_OP_REPLY:	//6
		//
		break;
	default:
		// unknown mns opcode in frame
		break;
	}

}

static void newdata(void)
{
	struct wan_packet *w = (struct wan_packet *)uip_appdata;
	w--;

	unsigned char DNet, DStn;
	unsigned char SNet, SStn;

	SNet = (unsigned char) *(uip_buf+28);	// octet 3 of source IP
	SStn = (unsigned char) *(uip_buf+29);	// octet 4 of source IP
	DNet = (unsigned char) *(uip_buf+32);	// octet 3 of destin IP
	DStn = (unsigned char) *(uip_buf+33);	// octet 4 of destin IP

	w->dstn = DStn;
	w->dnet = DNet;
	w->sstn = SStn;
	w->snet = SNet;

	process_msg (w, uip_len - AUNHDRSIZE);
}

static void newwandata(void)
{
	struct wan_packet *w = (struct wan_packet *)uip_appdata;

	switch (w->opcode) {
	case 0x00:
		process_msg (w, uip_len - AUNHDRSIZE - sizeof (struct wan_packet));
		break;
	}
}

/******************************************************************************/

/******************************************************************************/

/******************************************************************************/

/******************************************************************************/

void do_atp(unsigned char *Net, unsigned char *Stn)

{

	struct atp_block *atp;


	atp = ((struct atp_block *)&uip_buf[UIP_LLH_LEN]);

	atp->atpb_net = *Net;
	atp->atpb_station = *Stn;


}

/******************************************************************************/

/******************************************************************************/

void foward_packet(struct wan_packet *w, unsigned short pkt_len, uint8_t type)
{
	struct mns_msg *ah;
	struct Econet_Header *eh;

	/* if the destination econet network is the same as the network
	   definition on this network interface, change it to 0, the
	   local network */

	// use Econet header structure instead of scout packet structure
	// so the Port byte can easily be duplicated.
	ah = (struct mns_msg *)(uip_appdata);
	eh = (struct Econet_Header *)(uip_appdata+2);

	uint32_t handle = ah->mns_handle;
	uint8_t port = ah->mns_port;
	uint8_t cb = ah->mns_control | 0x80;

	eh->DSTN = w->dstn;
	eh->DNET = w->dnet;
	eh->SSTN = w->sstn;
	eh->SNET = w->snet;
	eh->CB = cb;
	eh->PORT = port;

	struct mbuf *mb = copy_to_mbufs (eh, pkt_len + 6);
	if (type == BROADCAST_DATA_FRAME)
	  enqueue_tx (mb);
	else
	  enqueue_aun_tx(mb, BUF, handle);
}

/*
aun_send_immediate() works in the same way as aun_send_packet() but, obviously, 
for immediate ops. You need to call back to adlc_immediate_complete(), again 
with either TX_OK or NOT_LISTENING.  If the result was TX_OK, and there was data 
returned (i.e. from machine peek) then you need to supply a buffer and length as well. 
*/
void aun_send_immediate (struct scout_packet *s, uint32_t dest_ip, uint16_t data_length)
{
  not_listening();
}


void aun_send_packet (uint8_t cb, uint8_t port, uint16_t src_stn_net, uip_ipaddr_t dest_ip, uint16_t data_length)
{
  struct mns_msg *ah;
  ah = (struct mns_msg *)(uip_appdata);

  ah->mns_opcode = DATA_FRAME;
  ah->mns_port = port;
  ah->mns_control = cb & 0x7f;
  ah->mns_status = 0;
  ah->mns_handle = ++s.handle;

  uint8_t src_stn = src_stn_net & 0xff;
  uint8_t src_net = src_stn_net >> 8;
  if (src_net == 0)
    src_net = eeprom.Econet_Network;

  BUF->srcipaddr[0] = uip_hostaddr[0];
  BUF->srcipaddr[1] = (src_stn << 8) | src_net;
  uip_ipaddr_copy(BUF->destipaddr, dest_ip);
  BUF->destport = BUF->srcport = HTONS(MNSDATAPORT);

  uip_udp_send(AUNHDRSIZE + data_length);
  do_uip_send();

  forwarding = 1;
  fwd_timeout = 20000;
}


/* 
   aun_send_broadcast() is for broadcast operations.
   There's no acknowledgement for these, you just need to send them off.
*/

void aun_send_broadcast (struct scout_packet *s, uint16_t data_length)
{
}


void aun_tx_complete (int8_t status, struct tx_record *tx)
{
  uint8_t length = 8;
  struct mns_msg *ah;
  uip_appdata = &uip_buf[UIP_IPUDPH_LEN + UIP_LLH_LEN];
  ah = (struct mns_msg *)(uip_appdata);

  uip_ipaddr_copy (BUF->destipaddr, tx->requestor_ip);
  uip_ipaddr_copy (BUF->srcipaddr, tx->target_ip);

  memset (ah, 0, sizeof (*ah));
  ah->mns_opcode = (status == TX_OK) ? DATA_FRAME_ACK : DATA_FRAME_REJ;
  ah->mns_handle = tx->requestor_handle;

  uip_udp_send(length);
  do_uip_send();
}

uint8_t aun_want_proxy_arp(uint16_t *ipaddr)
{
  if (ipaddr[0] == uip_hostaddr[0]
      && rTableEco[ipaddr[1] & 0xff])
    return 1;
  return 0;
}
