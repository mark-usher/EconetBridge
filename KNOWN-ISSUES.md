# Known issues

Found in November 2026 by reading the code (nothing was built, simulated or run on hardware). Line numbers are those of
the newest state: `avruip/` at SVN r8 on `main`, and `EcEthBridge/` on branch `Mark`. "certain" means the code is wrong as
written; "likely" depends on timing or load. None of these has been fixed; the history is kept as it was.

### 5.1 C bridge (`current-projects\trunk`, SVN r8)


| # | file:line | what goes wrong | sure | fix |
|---|---|---|---|---|
| C1 | `drivers/adlc_asm.S:463-474` `__vector_1` | The ISR saves only r24, r18, r19, SREG and X. Its call path changes r20 (`in saved_gicr, GICR`, line 195) and, on error paths, r25 (serial_tx). C code holding values in r20/r25 is corrupted at random on Econet traffic | certain | push/pop r20, r25 (and r0) in the ISR |
| C2 | `drivers/cs8900_asm.S:174-189, 224-225, 343-361`; `444-451` | `cs_software_reset`, `cs8900_poll_send` and `cs8900_poll_send_end` overwrite Y, the avr-gcc frame pointer (call-saved), which corrupts locals in `nic_init`/`nic_send`/`main`. `delay_65536` has no `ret` and falls into `cs8900SendPacketData` with Z=0 | certain | save/restore Y; add `ret` |
| C3 | `drivers/adlc_asm.S:611-613` | The local-network check compares the bridge's station number (`eeprom+6`) with tmp1, which still holds the network number, not `ECONET_RX_BUF[0]`. Net-0 frames addressed to the bridge (MachineType, IP data) are discontinued | certain | load `ECONET_RX_BUF+0` into tmp1 before the compare |
| C4 | `aun.c:462-464` | `else eh->CB = cb \| 0x80; eh->PORT = port;` has no braces, so `PORT` always overwrites the 0xFF marker. `do_tx_packet` never recognises an IMMEDIATE_OP_REPLY and sends the reply as a new immediate scout | certain | add braces |
| C5 | `aun.c:508` `switch (immediate_op && RXCB)` | The logical `&&` gives 0/1, so every reply takes the PEEK branch. That branch calls `adlc_poller()` from inside the uIP appcall (line 514), which can re-enter `uip_process` and corrupt `uip_buf` | certain | `&`; set a flag instead of polling inside the appcall |
| C6 | `adlc.c:620/633`, `559`, `573`; `aun.c:777`; `adlc_asm.S:567-584` | Buffer overruns. The length check omits `AUNHDRSIZE`, so the copy is 8 bytes past `uip_buf`. Broadcast and immediate paths have no length check at all. `copy_from_mbufs` is unbounded. The asm receive stores 2 bytes before the full check, writing 1 byte past `ECONET_RX_BUF` | certain | check against `UIP_BUFSIZE - offset - AUNHDRSIZE`; test the space before storing |
| C7 | `internet.c:113` | `(struct ec_arp *) ECONET_RX_BUF + offset` scales the offset by the struct size, so Econet ARP reads the wrong bytes | certain | `(struct ec_arp *)(ECONET_RX_BUF + offset)` |
| C8 | `adlc.c:571, 630` vs `adlc_asm.S:450-456, 527-546` | `adlc_state = BUSY_FORWARDING` is set while RIE is still on. The Rx-idle IRQ after the data frame resets the state to RX_IDLE, and a new frame overwrites `ECONET_RX_BUF` mid-handshake, so `make_scout_acknowledge` may ack the wrong station | likely | mask the receive IRQ or ignore it while forwarding; copy the addresses out first |
| C9 | `adlc_asm.S:415-419, 267-279`; `adlc.c:190-194, 219-223` | On clock loss DCD returns without clearing RX status. Level-triggered INT0 then gives an IRQ storm. `adlc_await_idle` and the scout-ack/data-ack waits have no timeout (`LINE_JAMMED` is never returned) | likely | clear status on DCD; bounded waits returning `LINE_JAMMED`/`NOT_LISTENING` |
| C10 | `aun.c:668`; `adlc.c:620/633` | `aun_send_packet` uses whatever `uip_appdata` holds. After TCP/ICMP input that is the TCP offset, so the AUN header lands 12 bytes late. `uip_udp_send` may copy from a stale `uip_sappdata` | likely | set `uip_appdata = uip_sappdata = &uip_buf[UIP_LLH_LEN+UIP_IPUDPH_LEN]` before building |
| C11 | `aun.c:757-781` `aun_tx_complete` | It never sets the UDP source/destination ports (reuses `uip_buf`), and acknowledges immediate ops as `DATA_FRAME_ACK` instead of `IMMEDIATE_OP_REPLY` | likely | set both ports to `HTONS(MNSDATAPORT)`; use opcode 6 for immediates |
| C12 | `adlc.c:352` with `aun.c:140` | `rTableEthIP = 0x0201 \| net<<16` sends AUN data to 1.2.net.stn whatever IP is configured, while immediates use `uip_hostaddr[0]` | likely | build the address from `uip_hostaddr` |
| C13 | `nic.c:85-96`; `internet.c:130/191/232`; `bridge.c:45`; `aun.c:466` | `mbuf` allocation failure (NULL) is not checked. `nic.c` then writes through NULL (into r2/r3 on the AVR). 8 TX records × 17 mbufs exceed the 128-mbuf pool | likely under load | check for NULL; drop or REJ |
| C14 | `cs8900_asm.S:469-472, 505-508`; `nic.c:325`, `nic.c:138` | `clr`/`sbc` depends on the caller's carry, and `nic.c:325` leaves C=1, so 2 extra bytes are moved on every transfer. `nic.c:138` ignores the "not ready" return of `cs8900_poll_send` | likely | `sub`/`sbc` or `clc` first; check the return value |
| C15 | `aun.c:337, 392-409` | On the WAN path `forward_aun_packet` reads `uip_appdata`, not `w`, so it is 6 bytes off. An unknown opcode calls `not_listening()` without checking `forwarding`, which resets the ADLC mid-receive. ACK/REJ never match handle or source IP | likely | pass the mns pointer; guard on `forwarding` and a handle match |

Also worth noting:
- `cs8900_asm.S:130-134`: FDX is written to LineCTL, not TestCTL (the same slip as EcEthBridge E12).
- `internet.c:186`: data frames are tagged `EcCb_ARP` (dead code; `forward_to_econet` is commented out at `uip.c:950`).
- `fwd_timeout`: counts main-loop passes, not time.

Not checked: `serial.c`, telnetd, the `uip_arp` changes, `eeprom_asm.S`, `xram.S`, `gcrt1.S`, Econet timing margins, and whether libgcc touches the `-ffixed` registers.

## EcEthBridge (assembler, working files of 2009-07, branch `Mark`)

| # | file:line | what goes wrong | sure | fix |
|---|---|---|---|---|
| E1 | `cs8900.asm:810` `mov YL, r18` | The send loop takes its length from r18/r19, which `ReadPPRegister` (BusST, line 770) has just overwritten. About 0x0118 bytes are sent instead of TxLength | certain | keep the length in a spare pair or on the stack across the BusST read |
| E2 | `cs8900.asm:825` `breq tx_data_loop` | At the end of an odd-length frame it branches back into the loop (the comment says "Finish"), so Z runs through all 64 KB, including the CS8900 and EGPIO ports | certain | `breq exit_send_data_loop` |
| E3 | `adlc.asm:27/99` vs ISR `adlc.asm:234-239` | `adlc_read`/`adlc_write` use r18 (GICR copy), X and T. `adlc_irq` saves only tmp, r16, r17 and SREG, so main-loop values (event, RxLength, tx length, X = lengthL/H) are corrupted on each ADLC interrupt | certain | push/pop r18, r19, XL, XH in `adlc_irq` |
| E4 | `cs8900.asm:276-282` LoadMAC | All six writes go to PacketPage 0x0158. The IA becomes `00 00 …`, so unicast frames to the bridge's MAC are not received | certain | write three 16-bit words to 0x0158, 0x015A and 0x015C |
| E5 | `cs8900.asm:854-858` read_TER | `andi tmp,0x08` tests the register-number field, which is always set. The wait ends at once and usually reports error 3 | certain | test TxOK (bit 0 of the high byte, r19) |
| E6 | `cs8900.asm:551-552` `add YH, r19` | The carry from the low byte is lost when computing the end of the receive buffer, so the copy can run on for up to 64 KB | certain | `adc YH, r19` |
| E7 | `bridge.asm:173-199`, `udp.asm:32-40`, `adlc.asm:337-354` | The Econet receive length is never bounded. The pointer can run from 0x4000 into the transmit buffer and the CS8900 at 0x8000, and `create_udp_packet` copies into internal SRAM at 0x232, overwriting `cs8900_state` (0x301-0x305) for frames over about 200 bytes | certain | check the limit in `process_rda`/`process_ap`, and build the UDP frame in XRAM |
| E8 | `adlc.asm:376-380` `check_dst_net` | `interesting_network` returns C=0, Z=0 for every net except 255, and `brne discontinue` rejects the frame. Only net-255 frames are forwarded; local-net traffic is always discontinued | certain | handle the C=0/Z=0 case (station/network test) instead of discontinuing |
| E9 | `cs8900.asm:746` | `PP_TxCmd_Force` (bit 8) is ORed into the low byte, so a stuck bid is never forced. Line 749 "clears" with r21, which holds the checksum high byte | certain | `ldi r17,0x01` for Force; store a zeroed register |
| E10 | `cs8900.asm:554-557` | In 8-bit I/O mode, RxStatus and RxLength come first from RTDATA. One byte is skipped and the copy starts in the status word, so the buffer starts with status/length and the last 3-4 frame bytes are left unread | likely | read the 4 header bytes, then the data |
| E11 | `adlc.asm:271-272`, `279-289` | `process_dcd` returns without CLRRX, which gives an interrupt storm on clock loss with level-triggered INT0. If SR2 shows no tested bit, the code falls into `process_fv` and reports a frame that never arrived | likely | clear RX status; `rjmp adlc_irq_ret` after the last test |
| E12 | `cs8900.asm:297-301` | "duplex" writes 0x40 to LineCTL instead of setting FDX in TestCTL (0x0118 bit 14). Full duplex is never set | certain | write MSB 0x40 to `PP_TestCTL` |

Also worth noting:
- `ip.asm:139-141`: the end-around carry is incomplete (off by one in rare cases).
- `bridge.asm:292-302`: Timer 1 fast PWM has no TOP or compare value, so no Econet clock is generated.
- `adlc.asm:409`: RTS can drive flag fill while the transmitter is idle.
- `serial.asm`: `serial_tx` clobbers r16 and tmp; `output_BadRunt`, `output_BadExtra` and `noclock` lack a `ret` (debug only).
- **Functional gaps, not slips:** no AUN 8-byte header is built, no scout ACK is ever sent on Econet, and Ethernet→Econet is not written.
- **Not checked:** the hardware. `adlc_read`/`adlc_write` wait for an INT1 edge on PD3, which `bridge.asm` marks "NotConnected", so every ADLC access may hang unless something pulses it.
