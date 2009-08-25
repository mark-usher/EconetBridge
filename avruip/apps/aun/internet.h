#include <stdint.h>

extern void internet_init(void);
extern void internet_poller(void);
extern void handle_ip_packet(uint8_t cb, uint16_t length, uint8_t offset);
extern void handle_port_b0(void);
