#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "adlc.h"
#include "aun.h"
#include "serial.h"
#include "uip.h"
#include "mbuf.h"
#include "internet.h"

#define BUF ((struct uip_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])

#define MAX_TX	8
#define ADLC_TX_RETRY_COUNT	16
#define ADLC_TX_RETRY_DELAY	128

unsigned char ECONET_RX_BUF[ECONET_RX_BUF_SIZE];
volatile register unsigned char adlc_state	asm("r15");

#define MACHINE_VENDOR          0x50
#define MACHINE_TYPE            0x50
#define MACHINE_VER_LOW         0x01
#define MACHINE_VER_HIGH        0x00

struct stats
{
  int frames_in;
  int short_scouts;
  int rx_bcast;
  int tx_attempts;
  int tx_collision;
  int tx_not_listening;
  int tx_net_error;
  int tx_line_jammed;
};

static struct stats stats;

struct rx_record
{
  uint8_t state, port, cb, stn, net, rx_port;
  int len;
  union {
    unsigned char *buf;
    void (*callback)(int);
  } d;
};

struct tx_record tx_buf[MAX_TX];

#define NORMAL_PACKET	0
#define BROADCAST	1
#define IMMEDIATE	2

#define NO_BUFFER	0xFF

extern uint16_t get_adlc_rx_ptr(void);

static uint8_t get_tx_buf(void)
{
  uint8_t i;
  for (i = 0; i < MAX_TX; i++)
  {
    if (tx_buf[i].mb == NULL)
      return i;
  }
  return NO_BUFFER;
}

extern uint8_t get_adlc_state(void);
extern uint8_t adlc_await_idle(void);
extern void adlc_tx_frame(struct mbuf *mb, unsigned char type);

static uip_ipaddr_t ip_target;
static uint8_t aun_cb, aun_port;

struct scout_mbuf
{
  struct mbuf *next, *prev;
  uint8_t length;
  uint8_t pad;
  uint8_t data[120];
};

static struct scout_mbuf scout_mbuf;

static uint8_t do_tx_packet(struct tx_record *tx) __attribute__ ((noinline));

static uint8_t do_tx_packet(struct tx_record *tx)
{
  unsigned char *buf = tx->mb->data;
  unsigned char type = NORMAL_PACKET;
  if (buf[0] == 0xff)
    type = BROADCAST;
  else if (buf[5] == 0)
    type = IMMEDIATE;

  stats.tx_attempts++;

  if (adlc_await_idle()) {
    stats.tx_line_jammed++;
    return LINE_JAMMED;
  }

  if (type == BROADCAST) {
    adlc_tx_frame (tx->mb, 1);
    adlc_ready_to_receive_scout();
    return TX_OK;
  }

  uint8_t extra_len = 0, do_4way = 0;
  if (type == IMMEDIATE && tx->mb->length >= 14) {
    extra_len = 8;
    uint8_t cb = buf[4] & RXCB;
    if (cb >= 0x02 && cb <= 0x05)
      do_4way = 1;
  }

  memcpy (scout_mbuf.data, tx->mb->data, 6 + extra_len);
  scout_mbuf.length = 6 + extra_len;
  adlc_tx_frame ((struct mbuf *)&scout_mbuf, 1);

  unsigned char state;
  do {
    state = get_adlc_state();
  } while (state != RX_IDLE && state != (RX_SCOUT_ACK | FRAME_COMPLETE));

  if (state == RX_IDLE) {
    adlc_ready_to_receive_scout();
    return NOT_LISTENING;
  }

  struct mbuf *mb = tx->mb;

  if (do_4way) {
    memcpy (scout_mbuf.data + 4, mb->data + 14, mb->length - 14);
    scout_mbuf.length = mb->length - 10;
    scout_mbuf.next = mb->next;
    mb = &scout_mbuf;
  }

  if (type == NORMAL_PACKET || do_4way) {
    adlc_tx_frame (mb, 0);

    do {
      state = get_adlc_state();
    } while (state != RX_IDLE && state != (RX_DATA_ACK | FRAME_COMPLETE));
  }

  scout_mbuf.next = NULL;

  if (type == IMMEDIATE && state != RX_IDLE) {
    uint16_t frame_length = get_adlc_rx_ptr() - (int)ECONET_RX_BUF;
    if (frame_length > 4) {
      tx->r_mb = copy_to_mbufs (ECONET_RX_BUF + 4, frame_length - 4);
    }
  }

  adlc_ready_to_receive_scout();

  return ((state & 0xf) == FRAME_COMPLETE) ? TX_OK : NET_ERROR;
}

uint8_t enqueue_tx(struct mbuf *mb)
{
  uint8_t i = get_tx_buf();
  if (i != NO_BUFFER)
  {
    struct tx_record *tx = &tx_buf[i];
    tx->retry_count = ADLC_TX_RETRY_COUNT;
    tx->retry_timer = 0;
    tx->mb = mb;
    tx->is_aun = 0;
    return i + 1;
  }
#ifdef DEBUG
  serial_tx_str ("no bufs!\n");
#endif
  return 0;
}

uint8_t enqueue_aun_tx(struct mbuf *mb, struct uip_tcpip_hdr *hdr, uint32_t handle)
{
  uint8_t i = enqueue_tx(mb);
  if (i) {
    struct tx_record *tx = &tx_buf[i-1];
    tx->is_aun = 1;
    tx->requestor_handle = handle;
    uip_ipaddr_copy (tx->requestor_ip, hdr->srcipaddr);
    uip_ipaddr_copy (tx->target_ip, hdr->destipaddr);
    tx->retry_count = 0;
  }
  return i;
}

static uint8_t should_bridge(struct scout_packet *s, uint32_t *ip_targetp)
{

  /* if destination is reachable fill ip_target address */

  if (rTableEthType[s->DNet] != NOT_ROUTABLE)
  {
    *ip_targetp = rTableEthIP[s->DNet] | ((unsigned long)(s->DStn) << 24);
    return 1;
  }
  else {
    return 0;
  }

}

static void make_scout_acknowledge(void) __attribute__ ((noinline));

static void make_scout_acknowledge(void)
{
  scout_mbuf.data[0] = ECONET_RX_BUF[2];
  scout_mbuf.data[1] = ECONET_RX_BUF[3];
  scout_mbuf.data[2] = ECONET_RX_BUF[0];
  scout_mbuf.data[3] = ECONET_RX_BUF[1];
}

static void make_and_send_scout(void)
{
  make_scout_acknowledge ();
  scout_mbuf.length = 4;
  adlc_tx_frame ((struct mbuf *)&scout_mbuf, 1);
}

static void do_local_immediate (uint8_t cb)
{
  switch (cb & RXCB)
  {
  case Econet_MachinePeek:
    make_scout_acknowledge ();
    scout_mbuf.data[4] = MACHINE_TYPE;
    scout_mbuf.data[5] = MACHINE_VENDOR;
    scout_mbuf.data[6] = MACHINE_VER_LOW;
    scout_mbuf.data[7] = MACHINE_VER_HIGH;
    scout_mbuf.length = AUNHDRSIZE;
    adlc_tx_frame ((struct mbuf *)&scout_mbuf, 1);
    break;
  }
}

void adlc_poller(void)
{
  if (adlc_state == RX_IDLE)
  {
    uint8_t i;
    for (i = 0; i < MAX_TX; i++)
    {
      struct tx_record *tx = &tx_buf[i];
      if (tx->mb)
      {
	if (tx->retry_timer == 0)
	{
	  uint8_t state = do_tx_packet (tx);
	  switch (state) {
	  default:
	    if (tx->retry_count--) {
	      tx->retry_timer = ADLC_TX_RETRY_DELAY;
	      break;
	    }
	  case TX_OK:
	  case LINE_JAMMED:
	    if (tx->is_aun)
	    {
	      aun_tx_complete (state, tx);
	    }
	    mbuf_free_chain(tx->mb);
	    tx->mb = NULL;
	    if (tx->r_mb) {
	      mbuf_free_chain(tx->r_mb);
	      tx->r_mb = NULL;
	    }
	    break;
	  }
	}
	else
	  tx->retry_timer--;
      }
    }
  }
  else if ((adlc_state & 0x0f) == FRAME_COMPLETE)
  {
    uint16_t frame_length = get_adlc_rx_ptr() - (int)ECONET_RX_BUF;
    stats.frames_in++;
    if (adlc_state == (RX_SCOUT | FRAME_COMPLETE))
    {
      if (frame_length < 6) {
        stats.short_scouts++;
	goto out;
      }
      struct scout_packet *s = (struct scout_packet *)ECONET_RX_BUF;
      if (s->DStn == 0xff)
      {
        stats.rx_bcast++;
	if (s->Port == FIND_SERVER_PORT)
	  handle_port_b0 ();
	if (s->Port == IP_PORT)
	  handle_ip_packet (ECONET_RX_BUF[4], frame_length, 6);
	if (s->Port == BRIDGE_PORT)
	  handle_port_9c (frame_length);
	else
	{
	  memcpy (uip_appdata + AUNHDRSIZE, ECONET_RX_BUF + 6, frame_length - 6);
// to do - the called procedure below in aun.c is empty
//	  aun_send_broadcast (s, frame_length - 6);
	}
      }
      else if (should_bridge (s, &ip_target[0]))
      {
	if (s->Port == IMMEDIATE_PORT)
	{
	  aun_send_immediate (s, ECONET_RX_BUF + 6, frame_length - 6);
	  return;
	}
      do_rx_data:
	aun_cb = s->ControlByte;
	aun_port = s->Port;
	make_and_send_scout ();
	adlc_ready_to_receive (RX_DATA);
	return;
      }
      else if (s->DStn == eeprom.Station && s->DNet == LOCAL_NETWORK)
      {
	if (s->Port == IMMEDIATE_PORT)
	{
	  do_local_immediate (s->ControlByte);
	}
	else if (s->Port == IP_PORT)
	{
	  goto do_rx_data;
	}
      }
    }
    else if (adlc_state == (RX_DATA | FRAME_COMPLETE))
    {
      if (aun_port == IP_PORT)
      {
	handle_ip_packet (aun_cb, frame_length, 4);
	make_and_send_scout ();
	goto out;
      }
      if ((frame_length - 4) > (UIP_BUFSIZE - ((char *)uip_appdata - (char *)uip_buf)))
      {
	serial_tx_str ("too big!");
	serial_crlf();
      }
      else
      {
	memcpy (uip_appdata + AUNHDRSIZE, ECONET_RX_BUF + 4, frame_length - 4);
	aun_send_packet (aun_cb, aun_port, *((uint16_t *)(ECONET_RX_BUF + 2)), ip_target, frame_length - 4);
	adlc_state = BUSY_FORWARDING;
	return;
      }
    }
  out:
    adlc_ready_to_receive_scout();
  }
}

void adlc_forwarding_complete(uint8_t result, uint8_t *buffer, uint8_t length)
{
  if (result == TX_OK)
  {
    make_scout_acknowledge ();
    if (length > 12)
      length = 12;
    memcpy (&scout_mbuf.data[4], buffer, length);
    scout_mbuf.length = 4 + length;
    adlc_tx_frame ((struct mbuf *)&scout_mbuf, 1);
  }

  adlc_ready_to_receive_scout();
}
