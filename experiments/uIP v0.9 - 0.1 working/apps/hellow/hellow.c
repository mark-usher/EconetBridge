
#include "uip.h"
#include "hellow.h"

static void aborted(void) {}
static void timedout(void) {}
static void closed(void) {}

static void connected(void) {

	struct example6_state *s = (struct example6_state *)uip_conn->appstate;
	s->state = STATE_WAITING;
	s->textlen = 0;
}

static void newdata(void) {

	struct example6_state *s = (struct example6_state *)uip_conn->appstate;
	if(s->state == STATE_WAITING) {
	s->state = STATE_HELLO;
	s->textptr = "Hello ";
	s->textlen = 6;
}

}
static void acked(void) {

	struct example6_state *s = (struct example6_state *)uip_conn->appstate;
	s->textlen -= uip_conn->len;
	s->textptr += uip_conn->len;

	if(s->textlen == 0) {
		switch(s->state) {
		case STATE_HELLO:
			s->state = STATE_WORLD;
			s->textptr = "world!\n";
			s->textlen = 7;
		break;
		case STATE_WORLD:
			uip_close();
		break;
		}
	}
}

static void senddata(void) {

	struct example6_state *s = (struct example6_state *)uip_conn->appstate;

	if(s->textlen > 0) {
		uip_send(s->textptr, s->textlen);
	}

}

void example6_init(void) {

  uip_listen(HTONS(1000));

}


void example6_app(void) {
	
	if(uip_aborted()) {
		aborted();
	}
	if(uip_timedout()) {
		timedout();
	}
	if(uip_closed()) {
		closed();
	}
	if(uip_connected()) {
		connected();
	}
	if(uip_acked()) {
		acked();
	}
	if(uip_newdata()) {
		newdata();
	}
	if(uip_rexmit() ||
		uip_newdata() ||
		uip_acked() ||
		uip_connected() ||
		uip_poll()) {
		senddata();
	}
}
