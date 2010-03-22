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


/* Test stuff */
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

#include "common.h"

void timerinit()
{
}

void handle_second()
{
}

byte newsecond = FALSE;

/* JD of 1/1/1900 */
const int j1900 = 2415021;

byte get_time_data(time_t now)
{
  struct tm *gmt = gmtime(&now);

  timedata.year  = gmt->tm_year - 100;
  timedata.month = gmt->tm_mon + 1;
  timedata.day   = gmt->tm_mday;
  timedata.hour  = gmt->tm_hour;
  timedata.min   = gmt->tm_min;
  timedata.bst   = 0; /* Dont need BST */
  return gmt->tm_sec;
}

byte get_ntpstratum()
{
  return 10;
}

void get_ntptime()
{
  byte secs;
#ifdef POSIX_TIME
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
#else
  struct timeval now;
  gettimeofday(&now, NULL);
#endif

  secs = get_time_data(now.tv_sec);
  ntp_high = get_ntp_secs() + secs;
  /* Convert nsec to a fraction of a second */
#ifdef POSIX_TIME
  ntp_low = (word32)(((double)now.tv_nsec / 1e9) * pow(2,32));
#else
  ntp_low = (word32)(((double)now.tv_usec / 1e6) * pow(2,32));
#endif
}
