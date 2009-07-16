

#include <avr/io.h>
#include <avr/sleep.h>


extern void serial_tx(uint8_t mask);
extern void serial_tx_hex(uint8_t mask);

int
main (void)
{
	CLKPR = 0x80;
	CLKPR = 0x0;

	// select 1 wait state for upper region at 0x8000-0xffff
	EMCUCR = (1 << SRL2);

	// set the MicroController Control Register
	MCUCR =  (1 << SRE) | (1 << ISC11) | (1 << SRW10); 
	/*		
	SRE	= 7		External SRAM Enable
	ISC111 		trigger on falling edge
	SRW10	= 6		External SRAM Wait State Select
	*/


	serial_tx(65);
	serial_tx(66);
	serial_tx(67);
	serial_tx(68);
	serial_tx(69);
	serial_tx(70);

	/* loop forever */

	for (;;)
        sleep_mode();

	return (0);
}
