/**
 * \addtogroup AUN
 * @{
 */

/**
 * \file
 *         An example of how to write uIP applications
 *         with protosockets.
 * \author
 *         Mark Usher
 */

/*
 * This is a short example of how to write uIP applications using
 * protosockets.
 */

/*
 * We define the application state (struct hello_world_state) in the
 * hello-world.h file, so we need to include it here. We also include
 * uip.h (since this cannot be included in hello-world.h) and
 * <string.h>, since we use the memcpy() function in the code.
 */
#include "aun.h"
#include "uip.h"
#include <string.h>

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */



/*
 * Declaration of the protosocket function that handles the connection
 * (defined at the end of the code).
 */

static void newdata(void);
static void check_entries(void);


static struct uip_udp_conn *resolv_conn = NULL;




/*---------------------------------------------------------------------------*/
/*
 * The initialization function. We must explicitly call this function
 * from the system initialization code, some time after uip_init() is
 * called.
 */
void
aun_init(void)
{
	int i;
	i=1;
 uip_listen(HTONS(32768));

}
/*---------------------------------------------------------------------------*/
/*
 * In AUN.h we have defined the UIP_APPCALL macro to
 * AUN_appcall so that this funcion is uIP's application
 * function. This function is called whenever an uIP event occurs
 * (e.g. when a new connection is established, new data arrives, sent
 * data is acknowledged, data needs to be retransmitted, etc.).
 */
void
aun_appcall(void)
{
 
 	serial_ok(1);

  if(uip_udp_conn->rport == HTONS(32768)) {
   if(uip_poll()) {
      check_entries();
    }
    if(uip_newdata()) {
      newdata();
    }

}
}
/*---------------------------------------------------------------------------*/




/*---------------------------------------------------------------------------*/
/** \internal
 * Called when new UDP data arrives.
 */
/*---------------------------------------------------------------------------*/
static void
newdata(void)
{
 	int i;
	i=1;
 	serial_ok(9);
}

static void 
check_entries(void)
{
	int i;
	i=1;
 	serial_ok(10);

}

