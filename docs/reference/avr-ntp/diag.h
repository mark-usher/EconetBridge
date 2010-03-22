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


/* DIAG */
#define DIAG_PORT 1099
#define DIAG_PORT 1099

/* Defines the structure of a diag packet */

struct DiagData {
  word16 good_frames;
  word16 busy_frames;
  word16 slip_bad_esc;
  word16 slip_overflow;
  word16 sent_frames;
  word16 dropped_frames;
  word16 ip_errors;
  word16 udp_errors;
  word16 service_errors;
  word16 ntp_bit_errors;
  word16 ntp_date_errors;
  word16 ntp_leap_seconds;
  int16  min_offset;
  int16  max_offset;
  word16 missed_minutes;
  word16 missed_seconds;
  word16 ntp_resets;
  word16 bad_offsets;
};

struct DiagConfig {
  byte reset;
};

union DiagUnion {
  struct DiagData data;
  struct DiagConfig config;
};

#define DIAG_PACKET_LENGTH (sizeof(union DiagUnion)) /* Length of diag data   */
#define DIAG_DATA_LENGTH (sizeof(struct DiagData)) /* Length of diag data   */
#define DIAG_CONFIG_LENGTH (sizeof(struct DiagConfig)) /* Length of diag data   */
