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


#if !defined PROTO_H
#define PROTO_H
/* IP */
#define IP_HEADER_LENGTH 20

#define VERSION_LEN (byte *)(buf+0)
#define SERVICE     (byte *)(buf+1)
#define IP_LENGTH   (word *)(buf+2)
#define IDENT       (word *)(buf+4)
#define FLAGS       (word *)(buf+6)
#define TTL         (byte *)(buf+8)
#define PROTOCOL    (byte *)(buf+9)
#define CHECKSUM    (word *)(buf+10)
#define SOURCE      (word32 *)(buf+12)
#define DEST        (word32 *)(buf+16)
#define IP_DATA     (buf+IP_HEADER_LENGTH)

/* ICMP */

#define ICMP_PROT 1
#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY 0

/* Fields */
#define ICMP_TYPE        (byte *)(IP_DATA+0)
#define ICMP_CODE        (byte *)(IP_DATA+1)
#define ICMP_CHECKSUM    (word *)(IP_DATA+2)
/* Data offset */
#define ICMP_DATA        (byte *)(IP_DATA+4)

/* UDP */

#define UDP_PROT 17
#define UDP_HEADER_LENGTH 8
/* Fields */
#define UDP_SOURCE   (word *)(IP_DATA+0)
#define UDP_DEST     (word *)(IP_DATA+2)
#define UDP_LENGTH   (word *)(IP_DATA+4)
#define UDP_CHECKSUM (word *)(IP_DATA+6)

/* Data offset */
#define UDP_DATA     (byte *)(IP_DATA+UDP_HEADER_LENGTH)

/* NTP */

#define NTP_PORT 123

/* Fields */
#define NTP_FLAGS                (byte *)(UDP_DATA+0)
#define NTP_STRATUM              (byte *)(UDP_DATA+1)
#define NTP_POLL                 (byte *)(UDP_DATA+2)
#define NTP_PRECISION            (byte *)(UDP_DATA+3)
#define NTP_ROOT_DELAY           (word32 *)(UDP_DATA+4)
#define NTP_ROOT_DISPERSION      (word32 *)(UDP_DATA+8)
#define NTP_REFID                (word32 *)(UDP_DATA+12)
#define NTP_REFERENCE_TIMESTAMP1 (word32 *)(UDP_DATA+16)
#define NTP_REFERENCE_TIMESTAMP0 (word32 *)(UDP_DATA+20)
#define NTP_ORIGINATE_TIMESTAMP1 (word32 *)(UDP_DATA+24)
#define NTP_ORIGINATE_TIMESTAMP0 (word32 *)(UDP_DATA+28)
#define NTP_RECEIVE_TIMESTAMP1   (word32 *)(UDP_DATA+32)
#define NTP_RECEIVE_TIMESTAMP0   (word32 *)(UDP_DATA+36)
#define NTP_TRANSMIT_TIMESTAMP1  (word32 *)(UDP_DATA+40)
#define NTP_TRANSMIT_TIMESTAMP0  (word32 *)(UDP_DATA+44)

#endif
