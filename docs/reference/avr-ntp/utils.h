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


#if !defined UTILS_H
#define UTILS_H

/* Header field manipulation */
/* Convert between network and host ordering */

static INLINE byte GET8(byte *p)
{
  return *p;
}

static INLINE void PUT8(byte *p, byte b)
{
  *p = b;
}

static INLINE void COPY8(byte *p1, const byte *p2)
{
  *p1 = *p2;
}

static INLINE word GET16(word *p)
{
  byte *bp = (byte *)p;
  return ((word)(*(bp+0)) << 8) | ((word)(*(bp+1)) << 0);
}


static INLINE void PUT16(word *p, word n) 
{
  byte *bp = (byte *)p;
  *(bp+0) = (byte)(n >> 8);
  *(bp+1) = (byte)(n >> 0);
}

static INLINE void CLEAR16(word *p) 
{
  *p = 0;
}

static INLINE void COPY16(word *p1, const word *p2)
{
  *p1 = *p2;
}

static INLINE void SWAP16(word *p1, word *p2)
{
  word t;
  t = *p1;
  *p1 = *p2;
  *p2 = t;
}

static INLINE void CLEAR32(word32 *p)
{
  *p = 0;
}

static INLINE void COPY32(word32 *p1, const word32 *p2)
{
  *p1 = *p2;
}

/* Some non-inlines */
word32 GET32(const word32 *p);
void PUT32(word32 *p, word32 n);
void NCOPY32(word32 *p1, const word32 *p2);
void SWAP32(word32 *p1, word32 *p2);

#endif

