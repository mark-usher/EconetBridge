#include "globals.h"


// PUBLIC MODULE VARIABLE DEFINITIONS 


sDefaults_t eeprom;

// was previously in cs8900.c - flag for Network driver 
uint8_t gPrevTxBidFail;

// previously in AUN.C - flags AUN forwarding a packet 
// and to wait for a response
volatile uint8_t forwarding;
