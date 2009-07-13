/*
 * Copyright (c) 2001, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Adam Dunkels.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: main.c,v 1.16 2006/06/11 21:55:03 adam Exp $
 *
 */

#include "compiler.h"

#include "uip.h"
#include "uip_arp.h"
#include "nic.h"

static struct uip_eth_addr ethaddr = {0x00,0x11,0x22,0x33,0x44,0x55};


#include "timer.h"



extern void serial_tx(uint8_t mask);
extern void serial_tx_hex(uint8_t mask);


#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */




/*****************************************************************************
*  Periodic Timeout Functions and variables
*
*  The periodic timeout rate can be changed depeding on your application
*  Modify these functions and variables based on your AVR device and clock
*    rate
*  The current setup will interrupt every 256 timer ticks when the timer
*    counter overflows.  timerCounter must count until 0.5 seconds have
*    elapsed
*****************************************************************************/

//#define TIMER_PRESCALE    1024
//#define F_CPU             14745600

//#define TIMERCOUNTER_PERIODIC_TIMEOUT (F_CPU / TIMER_PRESCALE / 2 / 256)

//static unsigned char timerCounter;


void AVR_init(void)
{

	CLKPR = 0x80;
	CLKPR = 0x0;

	// select 1 wait state for upper region at 0x8000-0xffff
	EMCUCR = (1 << SRL2);

	// set the MicroController Control Register
	MCUCR =  (1 << SRE) | (1 << ISC11) | (1 << SRW10);
	/*
	SRE	= 7		External SRAM Enable
	ISC111 		trigger on falling edge
	SRW10	= 6		External SRAM Wait State Select
	*/
}



/*---------------------------------------------------------------------------*/
int
main(void)
{
  int i;
  uip_ipaddr_t ipaddr;
  struct timer periodic_timer, arp_timer;

  AVR_init();
  clock_init();

  timer_set(&periodic_timer, CLOCK_SECOND / 2);
  timer_set(&arp_timer, CLOCK_SECOND * 10);

//  tapdev_init();

  nic_init();
  uip_init();

  uip_ipaddr(ipaddr, 192,168,0,10);
  uip_sethostaddr(ipaddr);
  uip_ipaddr(ipaddr, 192,168,0,1);
  uip_setdraddr(ipaddr);
  uip_ipaddr(ipaddr, 255,255,255,0);
  uip_setnetmask(ipaddr);

  uip_setethaddr(ethaddr);

//  httpd_init();

//  telnetd_init();

    hello_world_init();


  while(1) {
//    uip_len = tapdev_read();
    uip_len = nic_poll();

    if(uip_len > 0) {

      if(BUF->type == htons(UIP_ETHTYPE_IP)) {
	uip_arp_ipin();
	uip_input();
	/* If the above function invocation resulted in data that
	   should be sent out on the network, the global variable
	   uip_len is set to a value > 0. */
	if(uip_len > 0) {
	  uip_arp_out();
//	  tapdev_send();
	  nic_send();
	}
      } else if(BUF->type == htons(UIP_ETHTYPE_ARP)) {
	uip_arp_arpin();
	/* If the above function invocation resulted in data that
	   should be sent out on the network, the global variable
	   uip_len is set to a value > 0. */
	if(uip_len > 0) {
//	  tapdev_send();
	  nic_send();
	}
      }

    } else if(timer_expired(&periodic_timer)) {
      timer_reset(&periodic_timer);
      for(i = 0; i < UIP_CONNS; i++) {
	uip_periodic(i);
	/* If the above function invocation resulted in data that
	   should be sent out on the network, the global variable
	   uip_len is set to a value > 0. */
	if(uip_len > 0) {
	  uip_arp_out();
//	  tapdev_send();
	  nic_send();
	}
      }

#if UIP_UDP
      for(i = 0; i < UIP_UDP_CONNS; i++) {
	uip_udp_periodic(i);
	/* If the above function invocation resulted in data that
	   should be sent out on the network, the global variable
	   uip_len is set to a value > 0. */
	if(uip_len > 0) {
	  uip_arp_out();
//	  tapdev_send();
	  nic_send();
	}
      }
#endif /* UIP_UDP */

      /* Call the ARP timer function every 10 seconds. */
      if(timer_expired(&arp_timer)) {
	timer_reset(&arp_timer);
	uip_arp_timer();
      }
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
uip_log(char *m)
{
  printf("uIP log message: %s\n", m);
}
void
resolv_found(char *name, u16_t *ipaddr)
{
  u16_t *ipaddr2;

  if(ipaddr == NULL) {
    printf("Host '%s' not found.\n", name);
  } else {
    printf("Found name '%s' = %d.%d.%d.%d\n", name,
	   htons(ipaddr[0]) >> 8,
	   htons(ipaddr[0]) & 0xff,
	   htons(ipaddr[1]) >> 8,
	   htons(ipaddr[1]) & 0xff);
    /*    webclient_get("www.sics.se", 80, "/~adam/uip");*/
  }
}
#ifdef __DHCPC_H__

void
dhcpc_configured(const struct dhcpc_state *s)
{
  uip_sethostaddr(s->ipaddr);
  uip_setnetmask(s->netmask);
  uip_setdraddr(s->default_router);
  resolv_conf(s->dnsaddr);
}
#endif /* __DHCPC_H__ */


void
smtp_done(unsigned char code)
{
  printf("SMTP done with code %d\n", code);
}
void
webclient_closed(void)
{
  printf("Webclient: connection closed\n");
}
void
webclient_aborted(void)
{
  printf("Webclient: connection aborted\n");
}
void
webclient_timedout(void)
{
  printf("Webclient: connection timed out\n");
}
void
webclient_connected(void)
{
  printf("Webclient: connected, waiting for data...\n");
}
void
webclient_datahandler(char *data, u16_t len)
{
  printf("Webclient: got %d bytes of data.\n", len);
}
/*---------------------------------------------------------------------------*/
