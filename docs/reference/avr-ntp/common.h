/* $Revision: 1.11 $ */
/* (C) 2004 by Matthew Arcus */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#if !defined COMMON_H
#define COMMON_H

/* Features */
#define DO_ICMP
#define DO_DIAG
#define DO_NTP

#if defined AVR
#undef DEBUG
#define DEBUG 0
#else
#define DO_PRINT
#if !defined DEBUG
#define DEBUG 2
#endif
#endif

#if defined DO_PRINT
#include <stdio.h>
#include <string.h>
#define PRINTF(x) printf x
#define PRINT_IP_HEADER() print_ip_header()
#define PRINT_BUFFER(msg, length) print_buffer(msg, length)
#else
#define PRINTF(x)
#define PRINT_IP_HEADER()
#define PRINT_BUFFER(msg, length)
#endif

#include "types.h"

#if defined DO_DIAG
#include "diag.h"
#define DIAG(e) (e)
struct DiagData diagdata;
#else
#define DIAG(e)
#endif

#define ASSERT(x)
#define INLINE __inline__

/* 84 is just big enough for a default Unix ping with 56 bytes data. */
/* 76 is just big enough for a standard NTP packet. */

#if !defined BUFMAX
#define BUFMAX 128 /* The size of the input/output buffer */
#endif

/* Use only 8 bits for lengths if the buffer is small enough. */
#if BUFMAX < 256
typedef byte buflen_t;
#else
typedef word16 buflen_t;
#endif

/* Receive states */
enum RxState {
  RX_ACTIVE, /* Reading chars into the buffer */
  RX_IGNORE, /* Ignore input, eg. after buffer overflow */
  RX_ESCAPE, /* Just read a SLIP escape char */
  RX_FRAME   /* Frame received but not handled */
};

/* Transmit states */
enum TxState {
  TX_OFF,    /* TX interrupt disabled */
  TX_ACTIVE, /* Normal tranmission */
  TX_ESCAPE, /* Transmitting an escaped byte */
  TX_END,    /* Transmitted the last data */
  TX_FINAL,  /* Transmitted the frame byte */
};

/* SLIP */
#define SLIP_END 0xC0
#define SLIP_ESC 0xDB
#define ESC_END 0xDC
#define ESC_ESC 0xDD

/* Global data */
extern volatile byte     buf[BUFMAX];
extern volatile buflen_t rxptr;
extern volatile buflen_t txptr;
extern volatile buflen_t txend;
extern volatile buflen_t txpending;

extern volatile byte rx_state;
extern volatile byte tx_state;

/* Serial interface */
void init_io (void);

#if !defined AVR
void run_io (void);
byte byte_available(void);
#endif

/* NTP interface */

extern word32 ntp_low;
extern word32 ntp_high;
extern word32 ntp_reference;

/* Put current NTP time into the above variables */
void get_ntptime(void);
byte get_ntpstratum(void);

/* The data from the Rugby clock */
typedef struct {
  byte day;
  byte month;
  byte year;
  byte hour;
  byte min;
  byte bst;
} TimeData;
extern TimeData timedata;

word32 get_ntp_secs(void);

extern volatile byte newsecond; /* Flag to indicate a new seconds data */
void handle_second(void); /* Process the second data */
void timerinit(void);     /* Hardware initialization */

#endif
