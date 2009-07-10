/*

Simple web server example

*/
#include "uip.h"
#include "simples.h"
#include <string.h>


void simples_init(void) {
	uip_listen(HTONS(80));
	uip_listen(HTONS(81));
}

void simples_app(void) 
{

	struct simples_state *s;
	s = (struct simples_state *)uip_conn->appstate;

	char data_port_80, data_port_81;

	unsigned int datalen_port_80;
	unsigned int datalen_port_81;

	datalen_port_80 =5;
	datalen_port_81 =5;
	data_port_80 = "1234\n";
	data_port_81 = "abcd\n";

	if(uip_connected()) {
		switch(uip_conn->lport) {
		case HTONS(80):
			s->dataptr = data_port_80;
			s->dataleft = datalen_port_80;
			break;
		case HTONS(81):
			s->dataptr = data_port_81;
			s->dataleft = datalen_port_81;
			break;
		}
	
	uip_send(s->dataptr, s->dataleft);
	return;
	}

	if(uip_acked()) {
		if(s->dataleft < uip_mss()) {
			uip_close();
			return;
			}
		s->dataptr += uip_conn->len;
		s->dataleft -= uip_conn->len;
		uip_send(s->dataptr, s->dataleft);
	}
}
