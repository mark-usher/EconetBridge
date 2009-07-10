

#ifndef __HELLOW_H__
#define __HELLOW_H__

#include "uip.h"


#define STATE_WAITING 0
#define STATE_HELLO 1
#define STATE_WORLD 2


struct example6_state {
	u8_t state;
	char *textptr;
	int textlen;
};

void example6_app(void);
void example6_init(void);
static void aborted(void);
static void timedout(void);
static void closed(void);
static void connected(void);
static void newdata(void);
static void acked(void);


/* Finally we define the application function to be called by uIP. */
#ifndef UIP_APPCALL
#define UIP_APPCALL example6_app
#endif /* UIP_APPCALL */


#ifndef UIP_APPSTATE_SIZE
#define UIP_APPSTATE_SIZE (sizeof(struct example6_state))
#endif


#endif /* __HELLOW_H__ */
