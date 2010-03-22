/* $Revision: 1.2 $ */
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

word32 ntp_high;
word32 ntp_low;

/* JD of 1/1/1900 */
static const word32 j1900 = 2415021;

/* Return JD starting midday on the given date */
/* Algorithm due to Henry F. Fliegel and Thomas C. Van Flandern */
/* I guess this depends on the precise behaviour of division of -ve numbers */
word32 julian_day(int d, int m, int y)
{
  int jd = ( 1461 * ( y + 4800 + ( m - 14 ) / 12 ) ) / 4 +
           ( 367 * ( m - 2 - 12 * ( ( m - 14 ) / 12 ) ) ) / 12 -
           ( 3 * ( ( y + 4900 + ( m - 14 ) / 12 ) / 100 ) ) / 4 +
           d - 32075;
  return jd;
}

/* Two functions for getting the ntp second number of the start of the given date */
word32 test_ntp_sec0(byte day, byte month, byte year) 
{
  return 24 * 60 * 60 * (julian_day(day,month,year+2000) - j1900);
}

word32 test_ntp_sec1(byte day, byte month, byte year)
{
  timedata.year  = year;
  timedata.month = month;
  timedata.day   = day;
  timedata.hour  = 0;
  timedata.min   = 0;
  timedata.bst   = 0;
  return get_ntp_secs();
}

/* A test function */
/* This terminate at 1st March 2100 */
int main (int argc, char* argv[])
{
  byte day, month, year;
  for (year = 0; ; year++)
    for (month = 1; month <= 12; month++)
      for (day = 1; day <= 31; day++) {
	word32 sec0 = test_ntp_sec0(day,month,year);
	word32 sec1 = test_ntp_sec1(day,month,year);
	if (sec0 != sec1) {
	  printf ("%d %d %d: %lu %lu\n", day, month, year+2000, sec0, sec1);
	  goto end;
	}
      }
 end:
  return 0;
}

