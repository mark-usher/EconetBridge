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

int main (int argc, char* argv[])
{
  get_ntptime();
  printf ("%08lx.%08lx\n", ntp_high, ntp_low);
  return 0;
}
