/* $Revision: 1.3 $ */
/* (C) 2004 by Matthew Arcus */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#define BUFLENPOW 8
#define BUFLENMASK ((1<<BUFLENPOW)-1)
#define BUFMAX (1<<BUFLENPOW) /* Override definition in common.h */

#define STANDALONE

/* timer.c needs to be compiled differently for the standalone timer */
/* program than for inclusion in the main code, so we'll just include it here. */
/* A bit yukky, but there you go */

#include "timer.c"

SIGNAL (SIG_UART_DATA)
{
  if (txptr == txend) {
    /* Done tranmission, disable the interrupt */
    UCR &= ~_BV(UDRIE);
  } else {
    /* Send the byte */
    UDR = buf[txptr];
    txptr = incbufptr(txptr);
  }
}

void ioinit()
{
  DDRB =  0xff; /* all outputs */
  DDRD =  0x00; /* all inputs */
  PORTB = 0xff;

  /* UART setup */
  UCR |=   _BV(TXEN); /* Enable TX */
  UBRR =  12;    /* 19200 baud at 4MHz: faster speeds lost synch with a 4Mhz clock */
  timerinit();
  sei (); /* Enable interrupts */
}

static byte txbufferempty()
{
  byte result;
  cli();
  result = (txend == txptr);
  sei();
  return result;
}

int main (void)
{
  word32 h0 = 0;
  word32 l0 = 0;
  ioinit ();

  char message[] = "#Time Server 0.1\r";
  send_buffer(message, sizeof(message)-1);
  for (;;) {
    /* Show the rugby and UART signal */
    cli();
    PORTB = (PORTB & ~0x07) | (PIND & 0x07);
    sei();
    if (newsecond) {
      handle_second();
      newsecond = FALSE;
    } else if (ntp_state != NTP_OFF && txbufferempty()) {
      get_ntptime();
      if (!(ntp_high > h0 || (ntp_high == h0 && ntp_low > l0))) {
	static char message[] = "Meep!\r";
	cli();
	printhexword32(h0);
	send_char(':');
	printhexword32(l0);
	send_char('\r');
	printhexword32(ntp_high);
	send_char(':');
	printhexword32(ntp_low);
	send_buffer(message, sizeof(message)-1);
	sei();
      }
      h0 = ntp_high;
      l0 = ntp_low;
    }
  }

  return (0);
}
