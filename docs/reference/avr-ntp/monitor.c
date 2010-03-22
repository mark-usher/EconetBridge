/* $Revision: 1.6 $ */
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
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

#include "types.h"
#include "diag.h"

union DiagUnion diagunion;
#define diagdata diagunion.data
#define diagconfig diagunion.config

#define REPEAT_TIMEOUT 500
#define MAX_NUM_TRIES 10

static int udp_sock;
static int tcp_sock;
void sighandler (int signal)
{
  printf ("Closing down\n");
  close(udp_sock);
  close(tcp_sock);
  exit(0);
}

int setup_udp(char *iaddr)
{
  /* Make a UDP socket and connect to the remote device */
  int sock;
  struct sockaddr_in addr_local;
  struct sockaddr_in addr_remote;

  /* Make a UDP socket */
  sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("create udp socket failed");
    return -1;
  }
  /* Local address */
  addr_local.sin_family = AF_INET;
  addr_local.sin_port = htons(DIAG_PORT);
  addr_local.sin_addr.s_addr = htonl(INADDR_ANY);

  /* Remote address */
  addr_remote.sin_family = AF_INET;
  addr_remote.sin_port = htons(DIAG_PORT);
  addr_remote.sin_addr.s_addr = inet_addr(iaddr);

  if (bind (sock, (struct sockaddr *)&addr_local, sizeof(addr_local)) < 0) {
    perror("udp bind failed");
    return -1;
  }

  if (connect (sock, (struct sockaddr *)&addr_remote, sizeof(addr_remote)) < 0) {
    perror("udp connect failed");
    return -1;
  }
  return sock;
}

int setup_tcp()
{
  /* Make a TCP socket and start listening */
  int sock;
  int sopt;
  struct sockaddr_in addr_local;

  /* Make a TCP socket */
  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("create tcp socket failed");
    return -1;
  }
  /* Allow reuse of TCP socket */
  sopt = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&sopt, sizeof(sopt)) < 0) {
    perror ("setsockopt");
    exit(0);
  }
  /* Local address */
  addr_local.sin_family = AF_INET;
  addr_local.sin_port = htons(DIAG_PORT);
  addr_local.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind (sock, (struct sockaddr *)&addr_local, sizeof(addr_local)) < 0) {
    perror("tcp bind failed");
    return -1;
  }

  if (listen (sock, 10)) {
    perror("tcp listen failed");
    return -1;
  }
  return sock;
}

int get_diag(int sock, int reset)
{
  struct pollfd pollfds[1];
  int num_tries = 0;

  pollfds[0].fd = sock;
  pollfds[0].events = POLLIN;

  diagconfig.reset = reset;

  /* Now send packets with a 500 ms timeout until we get a response */
  do {
    if (num_tries == MAX_NUM_TRIES) break;
    if (send(sock, (void *)&diagunion, DIAG_PACKET_LENGTH, 0) < 0) {
      perror("sendto failed");
      return -1;
    }
    num_tries++;
  } while (poll(pollfds,1,REPEAT_TIMEOUT) == 0);

  if (num_tries == MAX_NUM_TRIES) {
    printf ("Can't contact device\n");
    return -1;
  }
  if ((pollfds[0].revents & POLLIN) == 0) {
    perror("poll error:");
    return -1;

  }
  if (recv(sock, (void *)&diagunion, DIAG_PACKET_LENGTH, 0) < 0) {
    perror("recv failed");
    return -1;
  }
  return 0;
}

#define BUFFSIZE 1024

#define CRLF "\r\n"

static const char http_header[] =
"HTTP/1.1 200 OK" CRLF
"Content-type: text/html" CRLF CRLF;

static const char prologue[] =
"<html><head>" CRLF
"<title>Matthew's time server</title>" CRLF
"<script language=\"javascript\">" CRLF
/* Just setting location.search doesn't work on IE */
"if (location.search != '') { location.replace(location.pathname); }" CRLF
"</script>" CRLF
"</head>" CRLF
"<body><h1>Matthew's Time Server</h1>" CRLF
"<input type=\"button\" value=\"Refresh\" onclick=\"location.reload(true)\">" CRLF
"<input type=\"button\" value=\"Reset\"   onclick=\"location.replace('?reset')\">" CRLF;

static const char epilogue[] =
"</body></html>" CRLF;

void handle_request (int client_sock)
{
  char buff[BUFFSIZE];
  char *buffptr = buff;
  char filename[256];
  int available = TRUE;
  int reset = FALSE;

  /* Read the request, let's not worry about extra-long requests, */
  /* we are only going to parse the first line. */
  buffptr += read(client_sock, buff, BUFFSIZE);
  *buffptr  = 0;
  printf ("%s\n", buff);

  /* Parse a GET request */
  if (sscanf(buff,"GET %255s", filename) != 1) {
    if (sscanf(buff,"POST %255s", filename) != 1) {
      return; /* Don't recognize this request */
    }
    reset = TRUE;
  }
  printf ("File %s requested\n", filename);

  if (get_diag(udp_sock, reset) != 0) {
    /* Probably couldn't contact device */
    available = FALSE;
  }
  buffptr = buff;
  buffptr += sprintf (buffptr, http_header);
  buffptr += sprintf (buffptr, prologue);
  buffptr += sprintf (buffptr, "<p>The server is %s" CRLF,
		      available?"up":"down");
  if (available) {
    int i;
    for (i = 0; i < DIAG_DATA_LENGTH/sizeof(word); i++) {
      *(((word *)&diagdata)+i)= ntohs(*(((word *)&diagdata)+i));
    }
    buffptr += sprintf (buffptr, 
			"<p><ul>"
			"<li>Good frames: %u" CRLF
			"<li>Busy frames: %u" CRLF
			"<li>Bad slip escape: %u" CRLF
			"<li>Slip overruns: %u" CRLF
			"<li>Sent frames: %u" CRLF
			"<li>Dropped frames: %u" CRLF
			"<li>IP errors: %u" CRLF
			"<li>UDP errors: %u" CRLF
			"<li>Service errors: %u" CRLF
			"<li>NTP bit errors: %u" CRLF
			"<li>NTP date errors: %u" CRLF
			"<li>NTP leap seconds: %u" CRLF
			"<li>Min offset: %d" CRLF
			"<li>Max offset: %d" CRLF
			"<li>Missed seconds: %u" CRLF
			"<li>Missed minutes: %u" CRLF
			"<li>NTP resets: %u" CRLF
			"<li>Bad offsets: %u" CRLF
			"</ul>" CRLF,
			diagdata.good_frames,
			diagdata.busy_frames,
			diagdata.slip_bad_esc,
			diagdata.slip_overflow,
			diagdata.sent_frames,
			diagdata.dropped_frames,
			diagdata.ip_errors,
			diagdata.udp_errors,
			diagdata.service_errors,
			diagdata.ntp_bit_errors,
			diagdata.ntp_date_errors,
			diagdata.ntp_leap_seconds,
			diagdata.min_offset,
			diagdata.max_offset,
			diagdata.missed_seconds,
			diagdata.missed_minutes,
			diagdata.ntp_resets,
			diagdata.bad_offsets
			);
  }
  buffptr += sprintf (buffptr,epilogue);
		     
  write(client_sock, buff, buffptr-buff);
}

int main(int argc, char* argv[])
{
  char* iaddr = "192.168.3.2";
  if (argc > 1) {
    iaddr = argv[1];
  }

  if ((udp_sock = setup_udp(iaddr)) < 0) {
    exit(0);
  }
  if ((tcp_sock = setup_tcp()) < 0) {
    exit(0);
  }
  /* Setup signal handler */
  if (signal(SIGINT, sighandler) == SIG_ERR) {
    perror("signal failed");
  }
  while (TRUE) {
    int client_sock;
    struct sockaddr_in client_addr;
    socklen_t size = sizeof(client_addr);
    if ((client_sock = accept(tcp_sock, 
			      (struct sockaddr *)&client_addr, 
			      &size)) < 0) {
      perror("accept failed\n");
      exit(0);
    }
    handle_request(client_sock);
    close(client_sock);
  }

  close(udp_sock);
  close(tcp_sock);
  return 0;
}

