/* $Revision: 1.5 $ */
/* (C) 2004 by Matthew Arcus */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <avr/io.h>
#include <avr/interrupt.h>

#include "common.h"
#include "avrtarget.h"

void init_io()
{
  /* UART setup */
  UBRRH =  0;
  UBRRL =  12; /* 19200 baud at 4MHz: faster speeds lost synch with a 4Mhz clock */
  
  UCSRA = _BV(U2X); /* Double baud rate, so we are really getting 38400 */
  UCSRB = _BV(RXEN) | _BV(TXEN);
  UCSRC = _BV(URSEL) | (3<<UCSZ0);

  /* 2313 values */
#if 0
  UCR = _BV(RXCIE) | _BV(TXCIE) | _BV(RXEN) | _BV(TXEN);
  UBRR =  12;   /*19200 baud at 4MHz: faster speeds lost synch with a 4Mhz clock */
#endif

  DDRB =  0xff; /* all outputs */
  DDRD =  0x00; /* all inputs */
  PORTB = 0xff;

  rx_on(); /* Enable receive interrupt */

  sei (); /* Enable interrupts */
}

