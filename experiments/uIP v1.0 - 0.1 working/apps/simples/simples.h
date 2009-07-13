

#ifndef __SIMPLES_H__
#define __SIMPLES_H__

#include "uip.h"

struct simples_state {
	char *dataptr;
	unsigned int dataleft;
};

void simples_init(void);
void simples_app(void);



#ifndef UIP_APPCALL
#define UIP_APPCALL     simples_app
#endif

#ifndef UIP_APPSTATE_SIZE
#define UIP_APPSTATE_SIZE (sizeof(struct simples_state))
#endif


#endif /* __SIMPLES_H__ */
