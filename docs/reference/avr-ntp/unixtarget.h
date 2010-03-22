/* $Revision: 1.4 $ */
/* (C) 2004 by Matthew Arcus */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


void tx_byte(byte b);
byte rx_byte(void);
void do_io(void);
void handle_rx(void);
void handle_tx(void);

static INLINE void rx_on(){}
static INLINE void rx_off(){}
static INLINE void tx_on(){}
static INLINE void tx_off(){}
static INLINE void cli(){}
static INLINE void sei(){}
