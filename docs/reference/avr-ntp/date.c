/* $Revision: 1.8 $ */
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

TimeData timedata;

/* The real functions we are going to use */
/* The number of NTP days before 1/1/2000 */
static const word16 ntp2000 = 36524;

/* The day number of the beginning of each month, (not a leap year) */
static const word16 monthstart[] =
  { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 }; 

/* Return the number of days since 1-1-2000 */
/* Year is 2000-based */
static word16 ntp_day ()
{
  /* Compute days since 1/1/2000. This function fails for earlier dates, */
  /* and will fail after February 28th 2100 which it will incorrectly */
  /* assume to be a leap year */
  word16 date;
  date = (timedata.year * 365) + ((timedata.year+3) >> 2);
  date += monthstart[timedata.month-1];
  date += timedata.day;
  /* Now day is 1 too big, so subtract it, unless there's been a leap day */
  if (timedata.month < 3 || (timedata.year & 0x3)) {
    date--;
  }
  return date;
}

word32 get_ntp_secs()
{
  word32 result = ntp2000; /* Start with the NTP day number of 1/1/2000 */
  result += ntp_day();     /* Add in days since 1/1/2000 */
  result = 24 * result + timedata.hour; /* Convert to hours */
  if (timedata.bst) {      /* Adjust for BST/DST if necessary */
    result--;
  }
  result = 60 * (60 * result + timedata.min); /* Convert to seconds */
  return result;
}
