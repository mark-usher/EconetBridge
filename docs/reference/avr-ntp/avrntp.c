/* $Revision: 1.10 $ */
/* (C) 2004 by Matthew Arcus */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "common.h"
#include "utils.h"
#include "proto.h"
#include "target.h"

/* Forward definitions */
static void do_ip(void);
static void do_udp(void);
static void do_icmp(void);
static void do_ntp(buflen_t udp_length);
static void do_diag(buflen_t udp_length);
static void reply_ip(buflen_t length);
static void reply_udp(buflen_t length);

/* Global data */

/* IO */
volatile byte     buf[BUFMAX];
volatile buflen_t rxptr = 0;
volatile buflen_t txptr = BUFMAX;
volatile buflen_t txend = BUFMAX;
volatile buflen_t txpending = 0;

volatile byte rx_state = RX_ACTIVE;
volatile byte tx_state = TX_OFF;

/* NTP */
word32 ntp_low = 0;
word32 ntp_high = 0;
word32 ntp_reference = 0;

/* Diag stuff */
#if defined DO_DIAG
volatile union DiagUnion diagunion;
#endif

/* SLIP */

/* Call to tell SLIP that we have finished processing the frame */
static INLINE void frame_done()
{
  cli();
  rx_state = RX_ACTIVE;
  rxptr = 0;
  rx_on(); /* Do platform-specific stuff, eg. reenable receive interrupts */
  sei();
}

static INLINE byte frame_available()
{
  return rx_state == RX_FRAME;
}

#if defined DO_PRINT
static void print_buffer (char *msg, buflen_t length)
{
  buflen_t i;
  printf("%s\n", msg);
  /* Print the bytes */
  for (i = 0; i < length; i++) {
    printf ("%02x%c", 
	    buf[i],
	    (i == length-1 || (i+1) % 20 == 0)?'\n':' ');
  }
}
#endif

static void send_buffer (buflen_t length)
{
  cli();
  /* We are still transmitting some data. */
  if (tx_state != TX_OFF) {
    /* Can txpending be non-zero? I rather hope not. */
    /* If txpending is non-zero, no more packets are accepted */
    txpending = length;
  } else {
    txptr = 0;
    txend = length;
    tx_state = TX_ACTIVE;
    tx_byte(SLIP_END);
    tx_on(); /* Enable tx interrupts */
  }
  sei();
}

/* Checksums */
static word checkaux (word chk, volatile byte *p, buflen_t length)
{
  byte chk1 = chk >> 8;
  byte chk0 = chk;
  buflen_t i;
  length >>= 1;
  for (i = 0; i < length; i++) {
    byte t = *p++; 
    chk1 += t;
    if (chk1 < t) {
      chk0++;
      if (chk0 == 0) {
	chk1++;
      }
    }
    t = *p++;
    chk0 += t;
    if (chk0 < t) {
      chk1++;
      if (chk1 == 0) {
	chk0++;

      }
    }
  }
  return (chk1 << 8) | chk0;
}

static word checksum(volatile byte *p, buflen_t length)
{
  word chk = checkaux(0,p,length);
  if (chk == 0xffff) return chk;
  return ~chk;
}

#if defined DO_PRINT
void print_ip_header()
{
  /* print_buffer("IP header: ", IP_HEADER_LENGTH); */
  printf ("Protocol: %d, Length: %d, From: %08lx, To: %08lx\n",
	  GET8(PROTOCOL), 
	  GET16(IP_LENGTH), 
	  GET32(SOURCE), 
	  GET32(DEST));
}
#endif

static void do_ip() {
#if DEBUG > 1
  PRINT_BUFFER("Received: ", rxptr);
#endif
  if (rxptr < 20) {
    PRINTF (("Packet too short: %d\n", rxptr));
    PRINT_BUFFER("Packet: ", rxptr);
    goto abort;
  } 
  else 
  {
    word flags;
    word chk;
    byte version, protocol;

    /* Check the first byte */
    version = GET8(VERSION_LEN);
    if (version != 0x45) {
      PRINTF (("Can't handle non-IPV4 or extended headers: %02x\n", version));
      goto abort;
    }
    chk = checksum(buf, 20);
    if (chk != 0xffff) {
      PRINTF (("Bad header checksum: 0x%04x\n", chk));
      goto abort;
    }
    PRINT_IP_HEADER();

    flags = GET16(FLAGS);
    if (flags != 0) {
      PRINTF (("Can't handle fragmentation: %04x\n", flags));
      goto abort;
    }
    protocol = GET8(PROTOCOL);
    switch (protocol) {
#if defined DO_ICMP
    case ICMP_PROT: {
      do_icmp();
      break;
    }
#endif
    case UDP_PROT: {
      do_udp();
      break;
    }
    default:
      PRINTF(("Unknown protocol: %d\n", protocol));
      goto abort;
    }
  }
  return;
 abort:
  PRINT_IP_HEADER();
  DIAG(diagdata.ip_errors++);
  return;
}

static void reply_ip(buflen_t length)
{
  word chk;

  /* Swap source and destination */
  SWAP32(SOURCE,DEST);

  CLEAR16(CHECKSUM);
  chk = checksum(buf, IP_HEADER_LENGTH);
  PUT16(CHECKSUM,chk);
  send_buffer(length); /* Initiate buffer sending */
}

#if defined DO_ICMP
static void do_icmp ()
{
  if (GET8(ICMP_TYPE) == ICMP_ECHO_REQUEST) {
    word chk;
    buflen_t ip_length = GET16(IP_LENGTH);

    chk = checksum(IP_DATA, ip_length-IP_HEADER_LENGTH);
    if (chk != 0xffff) {
      PRINTF (("ICMP checksum error: %04x\n", chk));
      DIAG(diagdata.service_errors++);
      return;
    }

    PUT8(ICMP_TYPE, ICMP_ECHO_REPLY);
    CLEAR16(ICMP_CHECKSUM);
    chk = checksum(IP_DATA, ip_length-IP_HEADER_LENGTH);
    PUT16(ICMP_CHECKSUM,chk);

    reply_ip(ip_length);
  }
}
#endif

/* UDP */

static word check_udp(buflen_t length)
{
  static const byte udp_prot[] = { 0x00, 0x11 };
  word chk = checkaux(0, IP_DATA, length);
  /* Check source and destination together */
  chk = checkaux(chk,(byte *)SOURCE,8);
  chk = checkaux(chk,(byte *)udp_prot,2);
  chk = checkaux(chk,(byte *)UDP_LENGTH,2);
  if (chk == 0xffff) return chk;
  return ~chk;
}

static void do_udp(void)
{
  word dest;
  buflen_t udp_length = GET16(UDP_LENGTH);
  word chk;
  chk = check_udp(udp_length);
  if (chk != 0xffff) {
    PRINTF (("Bad UDP checksum: %04x\n", chk));
    goto abort;
  }
  dest = GET16(UDP_DEST);
  switch (dest) {
#if defined DO_NTP
  case NTP_PORT:
    do_ntp(udp_length);
    break;
#endif
#if defined DO_DIAG
  case DIAG_PORT:
    do_diag(udp_length);
    break;
#endif
  default:
    PRINTF(("Unknown UDP port: %d", dest));
    goto abort;
  }
  return;
 abort:
  DIAG(diagdata.udp_errors++);
  return;
}

static void reply_udp(buflen_t length)
{
  /* Swap source and destination */
  SWAP16(UDP_SOURCE,UDP_DEST);

  CLEAR16(UDP_CHECKSUM);
  PUT16(UDP_CHECKSUM,check_udp(length));

  reply_ip(length + IP_HEADER_LENGTH);
}

/* UDP protocols */

#if defined DO_NTP
#define NTP_MODE_MASK 0x07
#define NTP_CLIENT_MODE 3
#define NTP_SERVER_MODE 4
#define LI_NORMAL 0
#define LI_ALARM 3
static void do_ntp(buflen_t udp_length)
{
#if 0
  word32 receive_high;
  word32 receive_low;
#endif
  static const byte MSF_REFID[] = "MSF\0";
  byte stratum = get_ntpstratum();
  byte precision = (stratum > 1) ? -10 : -16;
  byte flags;
  flags = GET8(NTP_FLAGS);
  if ((flags & NTP_MODE_MASK) != NTP_CLIENT_MODE) {
    PRINTF (("Can't handle non-mode 3 requests\n"));
    goto abort;
  }
  if (udp_length != 56) {
    PRINTF (("Can't handle extended NTP requests\n"));
    goto abort;
  }
#if 0
  get_ntptime();
  receive_high = ntp_high;
  receive_low = ntp_low;
#endif
  get_ntptime();
  /* Set flags LI = 0x03, mode = 3; */
  flags &= 0x38;
  flags |= NTP_SERVER_MODE;
  if (stratum > 0) {
    flags |= (LI_NORMAL << 6);
  } else {
    flags = (LI_ALARM << 6);
  }

  PUT8(NTP_FLAGS, flags);
  PUT8(NTP_STRATUM, stratum);
  PUT8(NTP_PRECISION,precision);
  CLEAR32(NTP_ROOT_DELAY);
  CLEAR32(NTP_ROOT_DISPERSION);
  COPY32(NTP_REFID, (word32 *)MSF_REFID);
  NCOPY32(NTP_REFERENCE_TIMESTAMP1, &ntp_reference); /* Reference timestamp */
  CLEAR32(NTP_REFERENCE_TIMESTAMP0); 
  COPY32(NTP_ORIGINATE_TIMESTAMP1, NTP_TRANSMIT_TIMESTAMP1);
  COPY32(NTP_ORIGINATE_TIMESTAMP0, NTP_TRANSMIT_TIMESTAMP0);
#if 0
  NCOPY32(NTP_RECEIVE_TIMESTAMP1, &receive_high);
  NCOPY32(NTP_RECEIVE_TIMESTAMP0, &receive_low);
#else
  NCOPY32(NTP_RECEIVE_TIMESTAMP1, &ntp_high);
  NCOPY32(NTP_RECEIVE_TIMESTAMP0, &ntp_low);
#endif
  NCOPY32(NTP_TRANSMIT_TIMESTAMP1, &ntp_high);
  NCOPY32(NTP_TRANSMIT_TIMESTAMP0, &ntp_low);

  /* Just to balance out times, we'll do the calculation again, maybe. */
  /* get_ntptime(); */
  reply_udp(udp_length);
  return;
 abort:
  DIAG(diagdata.service_errors++);
  return;
}
#endif

#if defined DO_DIAG
static void do_diag(buflen_t udp_length)
{
  int i;
  struct DiagConfig *diagconfig = (struct DiagConfig *)UDP_DATA;
  if (udp_length < UDP_HEADER_LENGTH + DIAG_PACKET_LENGTH) {
    PRINTF(("DIAG packet too small\n"));
    goto abort;
  }
  /* Now see if we need to reset */
  if (diagconfig->reset) {
    for (i = 0; i < sizeof(struct DiagData); i++) {
      ((byte *)&diagdata)[i] = 0;
    }
  }
  /* We assume the diagdata is just a sequence of 16 bit words */
  /* Send them out in network order */
  for (i = 0; i < DIAG_DATA_LENGTH/sizeof(word); i++) {
    PUT16(((word *)UDP_DATA)+i, *(((word *)&diagdata)+i));
  }
  reply_udp(udp_length);
  return;
 abort:
  DIAG(diagdata.service_errors++);
  return;
}
#endif

int main()
{
  timerinit();
  init_io();
  for (;;) {       /* loop for input */
    do_io();       /* target-specific actions */
    if (newsecond) {
      handle_second();
      newsecond = FALSE;
    } else if (frame_available()) {
      do_ip();
      frame_done();
    }
  }

  return 0;
}
