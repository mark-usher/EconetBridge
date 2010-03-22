/* $Revision: 1.4 $ */
/* (C) 2004 by Matthew Arcus */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include "common.h"
#include "unixtarget.h"

/* Unix interface for simple UDP/IP stack */

/* Serial link */
#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"

static int fd = -1;

byte rx_byte()
{
  byte b;

  if (read(fd,&b,1) != 1) {
    perror("Read failed");
    exit(0);
  }
  return b;
}

byte byte_available()
{
  int n;
  if (ioctl(fd, FIONREAD, &n) < 0) {
    perror("ioctl failed");
  }
  return n > 0;
}

void tx_byte (byte b)
{
  if (write(fd, &b, 1) != 1) {
    perror("Write failed");
  }
}

/* Simulate the interrupt system on the real target */
void do_io() 
{
  if (tx_state != TX_OFF) {
    handle_tx();
  }
  if (byte_available()) {
    handle_rx();
  }
}

void init_io ()
{
  struct termios newtio;

  fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY );

  if (fd <0) {
    perror(MODEMDEVICE);
    exit(0);
  }
  memset(&newtio, 0, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

  if (tcflush(fd, TCIFLUSH) < 0) {
    perror(MODEMDEVICE);
    exit(0);
  }
  if (tcsetattr(fd,TCSANOW,&newtio) < 0) {
    perror(MODEMDEVICE);
    exit(0);
  }
}
