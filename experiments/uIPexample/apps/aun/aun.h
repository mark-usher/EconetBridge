/**
 * \addtogroup apps
 * @{
 */

/**
 * \defgroup aun aun
 * @{
 *
 * A small example showing how to write applications with
 * \ref psock "protosockets".
 */

/**
 * \file
 *         Header file for AUN networking
 *         with protosockets.
 * \author
 *         Mark Usher
 */

#ifndef __AUN_H__
#define __AUN_H__

/* Since this file will be included by uip.h, we cannot include uip.h
   here. But we might need to include uipopt.h if we need the u8_t and
   u16_t datatypes. */


typedef int uip_udp_appstate_t;

//dummy as not using tcp at the moment
//typedef int uip_tcp_appstate_t;

/* Finally we define the application function to be called by uIP. */
void aun_appcall(void);

#ifndef UIP_UDP_APPCALL
#define UIP_UDP_APPCALL aun_appcall
#endif /* UIP_APPCALL */


//v0.9 only
#ifndef UIP_APPSTATE_SIZE
#define UIP_APPSTATE_SIZE (4)
#endif


//dummy for tcp
#ifndef UIP_APPCALL
#define UIP_APPCALL aun_appcall
#endif /* UIP_APPCALL */


#include "uipopt.h"

void aun_init(void);

#endif /* __AUN_H__ */
/** @} */
/** @} */
