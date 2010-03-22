/* $Revision: 1.6 $ */
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
#if defined AVR
#include "avrtarget.h"
#else
#include "unixtarget.h"
#endif

/* When compiling for AVR, this file defines the interrupt routines for */
/* UART data received and UART data register empty. */
/* When compiling for Unix it defines two regular functions for running */
/* the SLIP receive and transmit FSAs. */

#if defined AVR
SIGNAL (SIG_UART_RECV)
#else
void handle_rx()
#endif
{
  byte b = rx_byte();
  switch (rx_state) {
  case RX_FRAME: {
    /* Ignore all received bytes when we are processing a frame. */
    /* Just record that we saw a frame end. */
    /* This will count zero length frames too. */
    /* On AVR, we shouldn't get here as interrupts should be disabled */
#if defined DO_DIAG
    if (b == SLIP_END) {
      PRINTF(("Frame dropped - busy\n"));
      DIAG(diagdata.busy_frames++);
    }
#endif
    break;
  }
  case RX_IGNORE: {
    if (b == SLIP_END) {
      rx_state = RX_ACTIVE;
      PRINTF(("Frame dropped - ignored\n"));
      DIAG(diagdata.dropped_frames++);
    }
    break;
  }
  default: {
    /* state is ACTIVE or ESCAPE */
    if (rx_state == RX_ACTIVE) {
      /* Special character handling */
      switch(b) {
      case SLIP_ESC:
	rx_state = RX_ESCAPE;
	goto done;
      case SLIP_END:
	if (rxptr > 0) {
	  rx_state = RX_FRAME;
	  rx_off();
	  DIAG(diagdata.good_frames++);
	} /* Else stay in same state and wait for another packet */
	goto done;
      }
    }
    /* Find the right character to insert */
    if (rx_state == RX_ESCAPE) {
      rx_state = RX_ACTIVE;
      switch (b) {
      case ESC_ESC:
	b = SLIP_ESC;
	break;
      case ESC_END:
	b = SLIP_END;
	break;
      default:
	/* This shouldn't happen */
	PRINTF (("Unknown escaped character, aborting\n"));
	DIAG(diagdata.slip_bad_esc++);
	goto abort;
      }
    }
    /* Insert character if there is room, else restart */
    /* Note that we can simultaneously transmit and receive, though */
    /* receive will fail if it tries to overwrite data yet to be transmitted. */
    if (rxptr < txptr && txpending == 0) {
      buf[rxptr++] = b;
    } else {
      PRINTF (("Buffer full, aborting\n"));
      DIAG(diagdata.slip_overflow++);
      goto abort;
    }
  }
  }
 done:
  return;
 abort:
  /* Problem processing frame, ignore the rest of it and wait for another. */
  rxptr = 0;
  rx_state = RX_IGNORE; /* Skip the rest of this frame */
  return;
}

/* Data transmit buffer empty */
#if defined AVR
SIGNAL (SIG_UART_DATA)
#else
void handle_tx()
#endif
{
  ASSERT(tx_state != TX_OFF);
  switch (tx_state) {
  case TX_FINAL: {
    /* Nothing to do, just return */
    /* We reach here when the final SLIP_END of a frame has been sent. */
    if (txpending) {
      /* We have a frame (at the start of the buffer waiting to be sent) */
      txptr = 0;
      txend = txpending;
      tx_state = TX_ACTIVE;
      tx_byte(SLIP_END);
      txpending = 0;
    } else {
      tx_state = TX_OFF;
      tx_off(); /* Disable transmit interrupt */
    }
    break;
  }
  case TX_END: {
    /* Last byte has been transmitted, so send frame end */
    tx_state = TX_FINAL;
    DIAG(diagdata.sent_frames++);
    tx_byte(SLIP_END);
    break;
  }
  default: {
    byte b;
    b = buf[txptr];
    if (tx_state == TX_ACTIVE) {
      switch (b) {
      case SLIP_ESC:
      case SLIP_END:
	b = SLIP_ESC;
	tx_state = TX_ESCAPE;
	break;
      default:
	break;
      }
    } else {
      /* We are in TX_ESCAPE state */
      if (b == SLIP_ESC) {
	b = ESC_ESC;
      } else {
	b = ESC_END;
      }
      tx_state = TX_ACTIVE;
    }
    if (tx_state == TX_ACTIVE) {
      txptr++;
      if (txptr == txend) {
	txptr = BUFMAX;
	tx_state = TX_END;
      }
    }
    tx_byte(b);
  }
  }
}
