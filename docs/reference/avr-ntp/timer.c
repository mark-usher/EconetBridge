/* $Revision: 1.13 $ */
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
#include "avrtarget.h"

/* Some parameters that control the PLL */
#define ASHIFT
#define CGRAN 8
#define OGRAN 8
#define AGRAN 8
#define FIX   8
#define RESPONSE 0

/* If we get an offset bigger than this, reset everything */
#define MAXOFFSET 1000

#define CK64 3
#define CK1024 5

#define RUGBYIN 2 /* Pin on Port D for receiver input */

#define CLOCKRATE ((word32)4000000) /* 4 MHz clock */
#define NUMSAMPLES 40
#define SAMPLECOUNT ((byte)(((CLOCKRATE/NUMSAMPLES)+512)/1024))
#define SAMPLEOFFSET 50 /* Ad hoc offset from second mark to begin counting */
#define SECONDNUMBERZEROS ((7 * NUMSAMPLES/10)-1)

#define MAXCPSSAMPLES 16
#define DISCONNECT 256

#define CPS_LED 3
#define SECOND_LED 4
#define MINUTE_LED 5  /* On when a minute start is detected */
#define NTP_LED0 6    /* When a valid ntp time is found */
#define NTP_LED1 7    /* When a second valid ntp time is found */

enum SecondState {
  SECOND_START,
  SECOND_FIRST,
  SECOND_FOUND,
};

enum MinuteState {
  MINUTE_START,
  MINUTE_FOUND,
};

enum NtpState {
  NTP_OFF,        /* No NTP available */
  NTP_FIRST,      /* Have got our first date */
  NTP_OK,         /* Have got our second date and it was OK */
  NTP_BAD_DATE,   /* Have had an inconsistent date */
};

volatile byte newsecond = FALSE;

#if defined STANDALONE
volatile byte     buf[BUFMAX]; /* Output buffer */
volatile buflen_t txptr = 0;   /* Next char to transmit */
volatile buflen_t txend = 0;   /* End of bytes to be transmitted */
word32   ntp_low = 0;
word32   ntp_high = 0;
word32   ntp_reference = 0;    /* Last time we synched - just the second number */
#endif


/* Static data */
/* We count the number of zero (high level) samples to tell when a */
/* second starts for initial synchronization. */
static volatile word32   ntp_high0 = 0;
static volatile byte zeros = 0; 

static volatile byte second_number = 0; /* Which second are we in? */

static volatile byte abit;
static volatile byte bbit;

static volatile byte sample_number = 0; /* The sample within the second */

static volatile byte   cpsfound = 0;
static volatile word16 cpssamples = 0;

/* Build the current bit being read in here over 3 samples */
static volatile byte currentbit = 0;

/* Running average of clock cycles per second */
static volatile word32 clockspersec = 0;
/* Running average of second start offset */
static volatile int32 avoffset = 0;
static volatile word16 lasttick = 0;  /* Clocktime + lasttick = time since last tick */
static volatile int16 laststep = 0;

static volatile byte second_state = SECOND_START; /* Second detection state machine */
static volatile byte minute_state = MINUTE_START; /* Minute detection state machine */

/* Non-volatile data accessed in handle_second */
static byte abits[8]; /* A data stream */
static byte bbits[8]; /* B data stream */
static byte bitbyte;
static byte bitbit;
/* State machine for detecting 01111110 minute marker. */
/* May be reset to zero in an interrupt so (probably unnecessarily) made volatile. */
static volatile byte minute_marker_state = 0;
static volatile byte ntp_state = NTP_OFF;
static volatile word16 disconnected_count = 0;

/* Set this to non-zero to force stratum to particular level  */
static byte teststratum = 0;

static INLINE void reset_ntp()
{
  ntp_state = NTP_OFF;
  ledoff(NTP_LED0);
  ledoff(NTP_LED1);
}
  
static INLINE void reset_minute ()
{
  minute_state = MINUTE_START;
  minute_marker_state = 0;
  ledoff(MINUTE_LED);
}
  
static void reset_all ()
{
  second_state = SECOND_START;
  avoffset = 0; /* No stepping when unsynchronized */
  ledoff(SECOND_LED);
  reset_minute();
}

/* IO */

#if defined STANDALONE

/* Should always call these within cli() ... sei() */

static INLINE buflen_t incbufptr(buflen_t p)
{
  return (p+1) & BUFLENMASK;
}

/* Put a byte in the output buffer */
void send_char(byte b)
{
  buflen_t i = txend;
  txend = incbufptr(txend);
  if (txend == txptr) {
    txend = i; /* Buffer full, so reset txend */
  } else {
    buf[i] = b;
    UCR |= _BV(UDRIE); /* Enable the data empty interrupt */
  }
}

static void send_buffer(byte* b, buflen_t length)
{
  buflen_t i;
  for (i = 0; i < length; i++) {
    send_char(b[i]);
  }
}

/* Printing in hex */
static byte hexdigit(byte b) {
  if (b < 10) return '0' + b;
  else return 'A' + b - 10;
}

static void printhexbyte(byte b) {
  send_char(hexdigit(b >> 4));
  send_char(hexdigit(b & 0x0f));
}

static void printhexword16(word16 n) {
  printhexbyte(n >> 8);
  printhexbyte(n & 0xff);
}

static void printhexint16(int16 n) {
  if (n > 0) {
    send_char('+');
    printhexword16(n);
  } else if (n < 0) {
    send_char('-');
    printhexword16(-n);
  } else {
    send_char(' ');
    printhexword16(n);
  }
}

static void printhexword32(word32 n) {
  printhexword16(n >> 16);
  printhexword16(n & 0xffff);
}

void print_time_data()
{
    send_char('#');
    printhexbyte(timedata.day);
    send_char('-');
    printhexbyte(timedata.month);
    send_char('-');
    printhexbyte(timedata.year);
    send_char(' ');
    printhexbyte(timedata.hour);
    send_char(':');
    printhexbyte(timedata.min);
    if (timedata.bst) {
      send_char('S');
    }
    send_char('\n');
}

#else
#define send_char(x)
#define send_buffer(x,y)
#define printhexbyte(x)
#define printhexword16(x)
#define printhexint16(x)
#define printhexword32(x)
#endif

/* Can be in OK or BAD state. */
byte get_ntpstratum()
{
  if (ntp_state >= NTP_OK) {
    if (teststratum > 0) {
      return teststratum;
    } else {
      return 1;
    }
  } else {
    return 0;
  }
}

SIGNAL (SIG_OUTPUT_COMPARE1A)
{
  word16 newend;
  int16  step;
  word16 lastcps;

  newend = clockspersec >> FIX;

#if defined ASHIFT
  step = avoffset >> FIX;
#else
  step = avoffset / (1 << FIX);
#endif

  /* Get the point at which we last wrapped around */
  lastcps = OCR1AL;
  lastcps |= OCR1AH << 8;

  /* Add the last cycle in to lasttick. */
  lasttick += lastcps; /* Overflow? - shouldn't be a problem */

  /* And subtract some multiple of the current step from avoffset */
  /* This controls how quickly avoffset responds to the steps we take */
  avoffset -= (int32)step * RESPONSE;

  /* Step the clock by changing the wraparound point */
  /* avoffset is the "average" time between timer wraparound and the second */
  /* marker. If its positive then the second marker is occurring after the */
  /* wraparound, if negative then its happening before. */
  /* If step is positive, then we extend the wraparound interval, */
  /* ie. slow down the timer clock, since it is in advance of the true time. */
  /* If its negative, then we shrink the wraparound interval, */
  /* ie. speed up the timer clock, since it is behind the true time. */

  newend += step;
  laststep = step;

  send_char('#');
  printhexword16(step);
  send_char('\n');

  /* Set up the timer for the next second */
  OCR1AH = (newend >> 8) & 0xff;
  OCR1AL = newend & 0xff;

  newsecond = TRUE;

  /* Now check whether we are still in communication with the radio */
  if (second_state == SECOND_FOUND) {
    disconnected_count = 0;
  } else {
    if (disconnected_count >= DISCONNECT) {
      DIAG(diagdata.ntp_resets++);
      reset_ntp();
    } else {
      disconnected_count++;
    }
  }
  if (ntp_state != NTP_OFF) {
    ntp_high0++;
  }
}

/* Interrupt handler for External interrupt */
SIGNAL (SIG_INTERRUPT0) 
{
  /* Get the clocktime for the second */
  /* Do this first for best accuracy */
  word16 now = TCNT1L;        /* Read low then high */
  now |= TCNT1H << 8;

  GIMSK &= ~_BV(INT0);          /* Disable external interrupt */

  TCNT0 = (256 - SAMPLEOFFSET);  /* Restart sample timer */
  TIFR = _BV(TOV0);              /* Clear the Timer0 flag here, just in case. */

  sample_number = 0;

  switch (second_state) {

  case SECOND_START:
    if (cpsfound) {
      lasttick = -now;
      second_state = SECOND_FOUND;
      ledon(SECOND_LED);
    } else {
      /* Reset timer */
      TCNT1H = 0;                   /* Write high then low */
      TCNT1L = 0;
      second_state = SECOND_FIRST;
    }
    break;

  case SECOND_FIRST:
    TCNT1H = 0;                   /* Write high then low */
    TCNT1L = 0;
    /*clockspersec = (word32)now << 8; */
    clockspersec = 0x00F42000;
    second_state = SECOND_FOUND;
    ledon(SECOND_LED);
    break;

  case SECOND_FOUND: {
    word16 cycle;
    byte goodcycle = TRUE;
    if (!cpsfound) {
      /* Reset timer */
      TCNT1H = 0;                   /* Write high then low */
      TCNT1L = 0;
      cycle = now;
      cpssamples++;
      newsecond = TRUE;
      if (cpssamples >= MAXCPSSAMPLES /* (1 << (CGRAN+1)) */) {
	word16 end = clockspersec >> 8;
	/* Start the offset averaging */
	ledon(CPS_LED);
	cpsfound = TRUE;
	avoffset = 0;
	lasttick = 0;

	/* Now set up the Output Compare interrupt */
	OCR1AH = (end >> 8) & 0xff;
	OCR1AL = end & 0xff;
	TCCR1B |= _BV(WGM12); /* Clear counter on compare match */
	TIFR = _BV(OCF1A);    /* Clear outstanding interrupt, just in case */
	TIMSK |= _BV(OCIE1A); /* Enable interrupt - this is never disabled once set */
      }
    } else {
      word16 lastcps;
      int16 offset;

      lastcps = OCR1AL;
      lastcps |= OCR1AH << 8;

      /* In case the clock has wrapped around but the interrupt */
      /* handler hasn't been executed yet. */
      if ((int16)now >= 0 && (TIFR & _BV(OCF1A))) {
	now = lastcps;
      }

      cycle = lasttick + now;

      offset = now; /* Unsigned to signed conversion */
      if (offset < 0) {
	/* We haven't wrapped around yet, so calculate when we will */
	offset -= lastcps;
      } else {
	/* Take account of the step we have just made if necessary */
	offset -= laststep;
      }
#if defined DO_DIAG
      if (offset < diagdata.min_offset) {
	diagdata.min_offset = offset;
      }
      if (offset > diagdata.max_offset) {
	diagdata.max_offset = offset;
      }
#endif
#if 0
      if (offset < -MAXOFFSET || offset > MAXOFFSET) {
	goodcycle = FALSE;
	DIAG(diagdata.bad_offsets++);
	reset_all();
	reset_ntp();
      } 
      else
#endif
      {
#if defined ASHIFT
	avoffset = (avoffset<<OGRAN) - avoffset + ((int32)offset<<FIX);
	avoffset = (avoffset>>OGRAN);
#else
	avoffset = (avoffset*(1<<OGRAN)) - avoffset + ((int32)offset<<FIX);
	avoffset = (avoffset/(1<<OGRAN));
#endif
      }
#if defined STANDALONE
      printhexword32(clockspersec);
      send_char(' ');
      printhexword16(cycle);
      send_char(' ');
      printhexword16(now);
      send_char(' ');
      printhexword16(lasttick);
      send_char(' ');
      printhexword16(lastcps);
      send_char(' ');
      printhexword32(avoffset);
      send_char(' ');
      printhexint16(offset);
      send_char('\n');
#endif

      lasttick = -now; /* lasttick + clock = time since last tick = 0 ATM */
    }

    /* Running average of clocktime */
    if (goodcycle) {
      clockspersec = (clockspersec<<CGRAN) - clockspersec + ((word32)cycle<<FIX);
      clockspersec = (clockspersec>>CGRAN);
    }

    break;
  }
  }
}

SIGNAL (SIG_OVERFLOW0) /* Interrupt handler for Timer 0 */
{
  byte sample;
  sample = !((PIND & (1 << RUGBYIN)) >> 2); /* Pin high = 0, pin low = 1 */
  TCNT0 = (256 - SAMPLECOUNT); /* Count 130 cycles to generate 30 Hz interrupt */

  /* We are looking for the beginning of a second, wait for */
  /* the appropriate number of zero samples. */
  if (second_state == SECOND_START) {
    if (!sample) {
      zeros++;
    } else {
      zeros = 0;
    }
    if (zeros >= SECONDNUMBERZEROS) {
      zeros = 0;
      /* Enable external interrupt and wait for the fall */
      /* Clear any remembered event or bad things will happen */
      GIFR  = _BV(INTF0); /* Writing 1 clears IRQ flag, 0 leaves as is */
      GIMSK |= _BV(INT0); /* Enable external interrupt */
    }
  } else {
    /* We have successfully found a second marker so we can do more */
    /* complicated stuff with a sampling clock (roughly) synchronized  */
    /* to the seconds. */
    if (sample_number % 4 == 0) {
      currentbit = 0;
    } else {
      currentbit += sample;
    }
    switch (sample_number) {
    case 7:
#if defined DO_DIAG
      if (currentbit == 1 || currentbit == 2) {
	diagdata.ntp_bit_errors++;
      }
#endif
      currentbit >>= 1;
      /* Record the sampled A value */
      abit = currentbit;
      break;
    case 11:
#if defined DO_DIAG
      if (currentbit == 1 || currentbit == 2) {
	diagdata.ntp_bit_errors++;
      }
#endif
      currentbit >>= 1;
      /* Should sample bits 6,7,8 and use best of three */
      bbit = currentbit;
      break;
    case 38:
      /* 39 should be the last sample of the second. */
      /* Enable interrupt at this point. */
      /* Enable external interrupt and wait for the fall */
      /* Clear any remembered event or bad things will happen */
      GIFR  = _BV(INTF0);  /* Writing 1 clears IRQ flag, 0 leaves as is */
      GIMSK |= _BV(INT0);  /* Enable external interrupt */
      break;
    case 41:
      /* If we hit sample 41 without an interrupt, wrap around. */
      send_char('!');
      send_char('\n');
      GIMSK &= ~_BV(INT0);          /* Disable external interrupt */
      DIAG(diagdata.missed_seconds++);
      reset_all();
      sample_number = 2;
      break;
    }
    sample_number++;
  }
}

#if 0
byte extract_bits(byte *p, byte start_bit, byte length)
{
  byte b = 0;
  byte i;
  for (i = 0; i < length; i++) {
    byte index = (start_bit + i) >> 3;
    byte bit = (start_bit + i) % 8;
    b <<= 1;
    if (p[index] & (0x080 >> bit)) {
      b |= 0x01;
    }
  }
  return b;
}
#endif

byte extract_bits(byte *p, byte start, byte length)
{
  byte result;
  byte start_byte = start >> 3;
  byte end_byte = (start+length-1) >> 3;
  byte end_bit = (start+length-1) & 0x07;
  result = p[end_byte] >> (7 - end_bit);
  if (start_byte != end_byte) {
    result |= p[start_byte] << (end_bit+1);
  }
  return result & ((byte)(1<<length) - 1);
}

void timerinit()
{
  PORTD = _BV(RUGBYIN); /* enable internal pullup on Pin 2 (Rugby in) */
  /* TIMER0 setup */
  TCCR0 |= CK1024; /* CK/1024 */
  TIMSK |= _BV(TOIE0); /* Enable Timer0 overflow interrupt */
  /* TIMER1 setup */
  TCCR1B |= CK64; /* CK/64 - 16-bit second clock */
  /* INTERRUPT0 setup */
  MCUCR |= ISC10; /* Falling edge of PB2 causes interrupt */
}

void get_rugby_data(byte minute_end)
{
  timedata.day = 
    10 * extract_bits(abits, minute_end-60+30,2) + 
    extract_bits(abits, minute_end-60+32,4);

  timedata.month =
    10 * extract_bits(abits, minute_end-60+25,1) +
    extract_bits(abits, minute_end-60+26,4);

  timedata.year =
    10 * extract_bits(abits, minute_end-60+17,4) +
    extract_bits(abits, minute_end-60+21,4);

  timedata.hour =
    10 * extract_bits(abits, minute_end-60+39,2) +
    extract_bits(abits, minute_end-60+41,4);

  timedata.min =
    10 * extract_bits(abits, minute_end-60+45, 3) +
    extract_bits(abits, minute_end-60+48, 4);

  timedata.bst =
    extract_bits(bbits, minute_end-60+58, 1);
}

/* Have just read a second of data */
void handle_second()
{
  /* Run the minute marker FSA */
  switch (minute_marker_state) {
  case 0:
    if (!abit) {
      minute_marker_state++;
    }
    break;
  case 7:
    if (!abit) {
      /* We have found a minute marker! */
      minute_marker_state = 8;
    } else {
      minute_marker_state = 0;
    }
    break;
  default:
    if (abit) minute_marker_state++;
    else minute_marker_state = 1;
  }
  if (minute_state != MINUTE_START) {
    /* Include the second bit data in the relevant arrays */
    if (abit) {
      abits[bitbyte] |= bitbit;
    } else {
      abits[bitbyte] &= ~bitbit;
    }
    if (bbit) {
      bbits[bitbyte] |= bitbit;
    } else {
      bbits[bitbyte] &= ~bitbit;
    }
    bitbit >>= 1;
    if (bitbit == 0) {
      bitbit = (1 << 7);
      bitbyte++;
    }
#if 0 && defined STANDALONE
    cli();
    send_char('#');
    printhexbyte(second_number);
    send_char('\n');
    sei();
#endif
    second_number++;
    if (second_number == 62) {
      /* We seem to have missed the minute marker */
      /* So it's back to square one */
      DIAG(diagdata.missed_minutes++);
      reset_minute();
    }
  }
  if (second_state == SECOND_FOUND) {
    /* We have just synched with the clock, so record the time. */
    /* Don't care if NTP hasn't initialized as then we won't be sending this */
    ntp_reference = ntp_high0;
  }
  if (minute_marker_state == 8) {
    /* We have hit the end of a minute, so process the data */
    switch (minute_state) {
    case MINUTE_START:
      /* No minute end found yet */
      minute_state = MINUTE_FOUND;
      ledon(MINUTE_LED);
      break;
    case MINUTE_FOUND: {
	word32 secs;
	get_rugby_data(second_number);
#if defined STANDALONE
	cli();
	print_time_data();
	sei();
#endif
	secs = get_ntp_secs();
	if (ntp_state == NTP_OFF) {
	  /* Just starting up */
	  ntp_state = NTP_FIRST;
	  ntp_high0 = secs;
	  ledon(NTP_LED0);
	} else {
	  /* Check we have a sensible value */
	  if (secs == ntp_high0 + second_number - 60) {
	    if (second_number != 60) {
	      DIAG(diagdata.ntp_leap_seconds++);
	      ntp_high0 = secs;
	    }
	    ntp_state = NTP_OK;
	    ledon(NTP_LED1);
	  } else {
	    DIAG(diagdata.ntp_date_errors++);
	    switch (ntp_state) {
	    case NTP_FIRST:
	      /* If at first you don't succeed... */
	      ntp_high0 = secs;
	      break;
	    case NTP_OK:
	      ntp_state = NTP_BAD_DATE;
	      break;
	    case NTP_BAD_DATE:
	      /* Second bad date in a row, try this one */
	      ledoff(NTP_LED1);
	      ntp_high0 = secs;
	      ntp_state = NTP_FIRST;
	      break;
	    }
	  }
	}
      }
    }
    second_number = 0; /* Restart second count */
    bitbyte = 0;
    bitbit = (1 << 7);
    minute_marker_state = 0;
  }
}

/* Time to add on in tenths of a millisecond */
#define FUDGEFACTOR 170 /* Better value with low_latency serial port */
/* #define FUDGEFACTOR 240 */
#define FUDGE ((word32)FUDGEFACTOR * ((word32)1 << 16) / (word32)10000)

/* Get the current NTP time and put in ntp_high and ntp_low */
void get_ntptime ()
{
  word16 now;
  word16 cps;
  word32 temp32;

  cli(); /* Disable interrupts while we read the timer values */
  ntp_high = ntp_high0; /* Read current seconds value */
  now = TCNT1L;         /* and the current clock time */
  now |= TCNT1H << 8;
  cps = OCR1AL;
  cps |= OCR1AH << 8;

  if ((int16)now >= 0 && (TIFR & _BV(OCF1A))) {
    /* Clock has just wrapped around, but compare interrupt handler not run yet */
    ntp_high++;
  }
  sei(); /* reenable interrupts */

  /* It would appear that we can end up hitting the endstop here, */
  /* so clamp to the appropriate range. */
  if (now >= cps) {
    now = cps-1;
  }

  /* Divide clock time by our current value of cps to get a 16-bit NTP time */
  /* This 32-bit division may take a while. */
  temp32 = ((word32)now << 16)/(word32)cps;
  temp32 += FUDGE;         /* add in our fudge factor */
  ntp_high += temp32 >> 16;
  /* FIXME - should set the lower 16 bits to something randomish */
  ntp_low = temp32 << 16;
}
