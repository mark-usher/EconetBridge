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


#if !defined AVRTARGET_H
#define AVRTARGET_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>

/* For mega8535 */
#if defined __AVR_ATmega8535__
#define GIMSK GICR
#endif

#if defined __AVR_ATmega8535__ || defined __AVR_ATmega8__
#define UCR UCSRB
#define UBRR UBRRL
#endif

static INLINE void ledon(byte i) {
  byte oldsreg = SREG; /* Store old SREG - for I bit */
  cli(); /* Disable interrupts */
  PORTB &= ~(1 << i);
  SREG = oldsreg;
}

static INLINE void ledoff(byte i) {
  byte oldsreg = SREG; /* Store old SREG - for I bit */
  cli();
  PORTB |= (1 << i);
  SREG = oldsreg;
}

static INLINE void tx_byte(byte b)
{
  UDR = b;
}

static INLINE byte rx_byte()
{
  return UDR;
}

static INLINE void rx_on()
{
  UCSRB |= _BV(RXCIE); /* Enable receive interrupts */
}

static INLINE void rx_off()
{
  UCSRB &= ~_BV(RXCIE); /* Disable receive interrupts */
}

static INLINE void tx_on()
{
  UCSRB |= _BV(UDRIE); /* Enable receive interrupts */
}

static INLINE void tx_off()
{
  UCSRB &= ~_BV(UDRIE); /* Disable receive interrupts */
}

static INLINE void do_io()
{
  /* Show the state of the UART and rugby connection */
  cli();
  PORTB = (PORTB & ~0x07) | (PIND & 0x07);
  sei();
}

#endif
