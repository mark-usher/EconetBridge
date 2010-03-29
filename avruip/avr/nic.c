/*****************************************************************************
*  Module Name:       NIC Driver Interface for uIP-AVR Port
*
*  Created By:        Louis Beaudoin (www.embedded-creations.com)
*
*  Original Release:  November 16, 2003
*
*  Module Description:
*  Provides three functions to interface with a NIC driver
*  These functions can be called directly from the main uIP control loop
*  to send packets from uip_buf and uip_appbuf, and store incoming packets to
*  uip_buf
*
*
*****************************************************************************/

#include "uip.h"
#include "nic.h"
#include "serial.h"
#include "mbuf.h"
#include "aun.h"		// for declaration of MNSDATAPORT in debugging
#include <string.h>

#define IP_TCP_HEADER_LENGTH 	UIP_IPTCPH_LEN //40

#define TOTAL_HEADER_LENGTH (UIP_LLH_LEN + IP_TCP_HEADER_LENGTH)

//#define DEBUGrx 1
//#define DEBUGtx 
#define IP_MF   0x20		// More fragments bit
#define MTU	1500


#define BUF ((struct uip_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])

static struct frag_mbuf frag_mbuf;

extern uint8_t cs8900_poll_init (void);
extern void cs8900_poll_send (uint16_t length);
extern void cs8900SendPacketData (struct mbuf *mb, uint16_t length);
extern void cs8900_poll_send_end (void);
extern unsigned int cs8900_poll_retrieve (void);
extern void cs8900RetreivePacketData (struct mbuf *mb, uint16_t length);


void
nic_init (void)
{
  NICInit ();
}

/******************************************************************************
*
* NAME		: uip_to_mbufs          
*
* DESCRIPTION	: 
*
* INPUTS	: Nothing
*
* OUTPUTS	: Pointer to an mbuf
*
* NOTES		: Copies the data in uip_buf to a chain of mbufs with the
*		header in its own mbuf
*
* CHANGES	:
* REF NO	DATE		WHO	DETAIL
*					
*/

struct mbuf *
uip_to_mbufs (void)
{
  struct mbuf *mb;

  if (uip_len <= TOTAL_HEADER_LENGTH)
    {
      mb = copy_to_mbufs (uip_buf, uip_len);
    }
  else
    {
      // just put the header in the first mbuf so it can be reused
      mb = copy_to_mbufs (uip_buf, TOTAL_HEADER_LENGTH);

      // copy the data into a subsequent mbuf chain
      struct mbuf *mb2 =
	copy_to_mbufs (uip_appdata, uip_len - TOTAL_HEADER_LENGTH);
      
	// find the end of the mb chain and place in mbp
      struct mbuf *mbp = mb;
      while (mbp->next)
	{
	  mbp = mbp->next;
	}

      mbp->next = mb2;		// add the data mbuf chain to the last header mbuf
      mb2->prev = mbp;		// add the header to the previous ptr of the data
    }
  return mb;
}

/******************************************************************************
*
* NAME		: send_frag        
*
* DESCRIPTION 	: 
*
* INPUTS	: 
*
* OUTPUTS	: Nothing
*
* NOTES		:   
*
* CHANGES	:
* REF NO	DATE		WHO	DETAIL
*					
*/

static void
send_frag (struct mbuf *mb, uint16_t length)
{

#ifdef DEBUGtx
	serial_tx_str ("xmit ");
	serial_tx_hex (length >> 8);
	serial_tx_hex (length);
	serial_crlf ();

  if (*(mb->data + 0x22) == HTONS (MNSDATAPORT))
    {				// only output AUN packets
      serial_tx_str ("eth tx ");
      serial_packet ((unsigned short) &mb->data + 42, length - 42);	// write out the payload
      serial_crlf ();		// add extra CR LF for spacing
    }
//      memset (&mb->data[0], 0xff, 6);	// send to broadcast address

#endif

  NICBeginPacketSend (length);
  while (length)
    {
      uint16_t this_length = mb->length;
      if (this_length > length)
	this_length = length;

      NICSendPacketData ((struct mbuf *) &mb->data, this_length);
      length -= this_length;

      // get the next mb in the chain
      mb = mb->next;
    }
  NICEndPacketSend ();
}


/******************************************************************************
*
* NAME	:	nic_send	          
*
* DESCRIPTION :	Sends data from the memory buffer and sends on the NIC
*
* INPUTS	:	pointer to mbuf
*
* OUTPUTS	:	Nothing
*
* NOTES	:   
*
* CHANGES	:
* REF NO	DATE		WHO	DETAIL
*					
*/

void
nic_send (struct mbuf *mb)
{

struct uip_tcpip_hdr *pkt_start_IPH; // temp pointer to the pktstart IP Header

  // if no mbuf is passed then the data is taken from the uip_buf buffer and 
  // placed into mbufs. The first in the chain will be placed in the pointer mb

  if (mb == NULL)
    {
      mb = uip_to_mbufs ();
    }

  // set counters to handle fragmentation
  uint16_t length = 0, offset = 0;

  // set temporary pointers, all initialised to the first in the chain
  struct mbuf *mbp = mb, *pkt_start_mb = mb;

  // set pkt_start_IPH as a temp pointer to the current IP header
  pkt_start_IPH = (struct uip_tcpip_hdr *) &pkt_start_mb->data[UIP_LLH_LEN];


  // get the ethernet header and IP header from the first memory buffer
  // and store in frag_mbuf temporary area to reuse
  memcpy (&frag_mbuf.data[0], pkt_start_mb->data, UIP_LLH_LEN + UIP_IPH_LEN);


  // loop through the mbuf chain starting at mb
  while (mbp)
    {
      // will adding the length of this mbuf exceed the ethernet packet length
      if ((length + mbp->length) >= MTU)
	{
	/* packet needs fragmenting */

	// set the MF - more fragments bit in the IP header
	pkt_start_IPH->ipoffset[0] = IP_MF;

	// work out the payload size for the packet so far (UDP header + data)
	uint16_t payload_length = length - (UIP_LLH_LEN + UIP_IPH_LEN);

	// the payload must be divided into portions on an 8 octet (64 bit) boundary
        // so a maximum of seven bytes may overrun.
	uint8_t overdone = payload_length & 7;

        length -= overdone;		// trim the length

	// update the IP header length
	pkt_start_IPH->len[0] = ((length - UIP_LLH_LEN) >> 8);
	pkt_start_IPH->len[1] = ((length - UIP_LLH_LEN) & 0xff);

	// redo header checksum
	// copy header to uip_buff, and use the uip routine
	memcpy (&uip_buf, &pkt_start_mb->data, (UIP_LLH_LEN + UIP_IPH_LEN));
	BUF->ipchksum = 0;
	BUF->ipchksum = ~(uip_ipchksum ());
	pkt_start_IPH->ipchksum = BUF->ipchksum;


	// send the current fragment with the trimmed length
	send_frag (pkt_start_mb, length);

	  length = UIP_LLH_LEN + UIP_IPH_LEN + overdone;	// set new length

	// copy into the end of the data area of temp header, the last bytes
	// (overdone). This will be the start of the next packet.
	memcpy (&frag_mbuf.data[(UIP_LLH_LEN + UIP_IPH_LEN)],
	  		mbp->prev->data + mbp->prev->length - overdone, overdone);

	  frag_mbuf.length = length;			// set the mbuf length
	  frag_mbuf.next = mbp;				// set the next in the chain

	  pkt_start_mb = (struct mbuf *) &frag_mbuf;	// set the start mbuf for the next packet
	  offset += (payload_length - overdone);	// fragment counter

	  // reset pkt_start_IPH as a temp pointer to the current IP header as mbuf 
	  // has changed
	  pkt_start_IPH = (struct uip_tcpip_hdr *) &pkt_start_mb->data[UIP_LLH_LEN];

	  pkt_start_IPH->ipoffset[0] = ((offset / 8) >> 8) | IP_MF;
	  pkt_start_IPH->ipoffset[1] = (offset / 8);

	}

	// otherwise the length will be ok with this packet
      length += mbp->length;		// add length to the running total
      mbp = mbp->next;			// check the next mbuf
   }


  // last mbuf in the data chain reached
  // reset pkt_start_IPH as a temp pointer to the current IP header as mbuf may have changed
  pkt_start_IPH = (struct uip_tcpip_hdr *) &pkt_start_mb->data[UIP_LLH_LEN];

 // was the fragment flag set
if (pkt_start_IPH->ipoffset[0] && IP_MF)
{
	  pkt_start_IPH->ipoffset[0] &= ~IP_MF;		// turn off More Fragments bit as last packet

	// update the IP header length
	pkt_start_IPH->len[0] = ((length - UIP_LLH_LEN) >> 8);
	pkt_start_IPH->len[1] = ((length - UIP_LLH_LEN) & 0xff);

	// redo header checksum
	// copy header to uip_buff, and use the uip routine
	memcpy (&uip_buf, &pkt_start_mb->data, (UIP_LLH_LEN + UIP_IPH_LEN));
	BUF->ipchksum = 0;
	BUF->ipchksum = ~(uip_ipchksum ());
	pkt_start_IPH->ipchksum = BUF->ipchksum;
}

  send_frag (pkt_start_mb, length);	// send the packet
  mbuf_free_chain (mb);			// release the chain

}


/******************************************************************************
*
* NAME	:	nic_poll	          
*
* DESCRIPTION : 
*
* INPUTS	: 
*
* OUTPUTS	:	Nothing
*
* NOTES	:   
*
* CHANGES	:
* REF NO	DATE		WHO	DETAIL
*					
*/

#if UIP_BUFSIZE > 255
unsigned short
nic_poll (void)
#else
unsigned char
nic_poll (void)
#endif				/* UIP_BUFSIZE > 255 */
{
  unsigned int packetLength;

  packetLength = NICBeginPacketRetreive ();

  // if there's no packet or an error - exit without ending the operation
  if (!packetLength)
    return 0;

  // drop anything too big for the buffer
  if (packetLength > UIP_BUFSIZE)
    {
      NICEndPacketRetreive ();
      return 0;
    }

  // copy the packet data into the uIP packet buffer
  NICRetreivePacketData ((struct mbuf *) &uip_buf, packetLength);

#ifdef DEBUGrx

  if (*(uip_buf + 0x22) == HTONS (MNSDATAPORT))
    {				// only output AUN packets
      serial_tx_str ("eth rx \r\n");
      serial_packet ((unsigned short) &uip_buf + 42, packetLength - 42);	// write out the payload
    }
#endif

  NICEndPacketRetreive ();

#if UIP_BUFSIZE > 255
  return packetLength;
#else
  return (unsigned char) packetLength;
#endif /* UIP_BUFSIZE > 255 */

}
