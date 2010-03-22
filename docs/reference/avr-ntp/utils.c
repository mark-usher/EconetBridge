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


#include "common.h"
#include "utils.h"

#if !defined AVR
word32 GET32(const word32 *p)
{
  byte *bp = (byte *)p;
  return ((word32)(*(bp+0)) << 24) | 
         ((word32)(*(bp+1)) << 16) |
         ((word32)(*(bp+2)) << 8) | 
         ((word32)(*(bp+3)) << 0);
}

void PUT32(word32 *p, word32 n) 
{
  byte *bp = (byte *)p;
  *(bp+0) = (byte)(n >> 24);
  *(bp+1) = (byte)(n >> 16);
  *(bp+2) = (byte)(n >> 8);
  *(bp+3) = (byte)(n >> 0);
}
#endif

/* Copy p2 to p1, putting in Network Order on the way */
void NCOPY32(word32 *p1, const word32 *p2) 
{
  byte *bp1 = (byte *)p1;
  byte *bp2 = (byte *)p2;
  
  *(bp1+0) = *(bp2+3);
  *(bp1+1) = *(bp2+2);
  *(bp1+2) = *(bp2+1);
  *(bp1+3) = *(bp2+0);
}

void SWAP32(word32 *p1, word32 *p2)
{
  word32 t;
  t = *p1;
  *p1 = *p2;
  *p2 = t;
}

#if 0
void CLEAR32(word32 *p) 
{
  byte *bp = (byte *)p;
  *(bp+0) = 0;
  *(bp+1) = 0;
  *(bp+2) = 0;
  *(bp+3) = 0;
}

void COPY32(word32 *p1, const word32 *p2)
{
  *p1 = *p2;
}
#endif
