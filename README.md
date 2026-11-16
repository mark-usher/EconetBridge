# EconetBridge

**A hardware bridge between Acorn Econet and Ethernet (TCP/IP), built on an Atmel ATmega162.**

Acorn's Econet was the local network of BBC Micros, Masters and Archimedes machines in the 1980s and 1990s. AUN (Acorn
Universal Networking) later carried the same Econet protocols over Ethernet and IP. EconetBridge is a small board that
connects the two: Econet frames from the old machines go out as AUN packets over UDP on Ethernet, and back. With it, a
BBC Micro on Econet could reach a file server or another station on an Ethernet network.

The project was written in 2009 and 2010 by **Phil Blundell** and **Mark Usher**, published as the BerliOS project
`ecobridge` (CVS, then Subversion). BerliOS closed in 2014 and its history was lost; this repository rebuilds that history
from the copies that survived. It is a historic project: development stopped in April 2010, and the code has known bugs
(see [KNOWN-ISSUES.md](KNOWN-ISSUES.md)).

## Hardware

- **MCU:** Atmel ATmega162 with external RAM (XRAM).
- **Econet side:** a Motorola MC6854 ADLC, the same chip Acorn used.
- **Ethernet side:** a Cirrus Logic CS8900A (the driver in use); drivers for the Realtek RTL8019 and ASIX AX88796 exist
  too.
- Phil Blundell's design drawings are in [docs/design/](docs/design/); datasheets and other reference material in
  [docs/reference/](docs/reference/).

## What is where

| Folder | What it is |
|---|---|
| `asm/` | Phil Blundell's first firmware: one assembler file for the ATmega162 (2009-04), and the version published on BerliOS (2009-06) |
| `EcEthBridge/` | the modular assembler bridge (2009-06/07); its last working state is on branch `Mark` |
| `experiments/` | test programs written along the way (memory, serial, timer, CS8900, uIP trials), 2009-06/07 |
| `avruip/` | **the C bridge**: the uIP TCP/IP stack on the AVR, with the AUN application (`apps/aun/`), the ADLC and Ethernet drivers (`drivers/`), and AVR support (`avr/`). This is the main line, 2009-07 to 2010-04 |
| `docs/` | design drawings, photos of the hardware, and reference material |

## Building

Two ways, both kept as they were:

- **AVR Studio 4** (as Mark built it): open `avruip/ecobridge.aps` (the last state) or `avruip/avruip.aps`. The assembler
  projects have their own `.aps` files in `EcEthBridge/` and under `experiments/`.
- **avr-gcc and make** (as Phil built it): in `avruip/`, run `make`. You need `avr-gcc`, `avr-libc` and `make`
  (on Debian or Raspberry Pi OS: `sudo apt install gcc-avr avr-libc make`). The Ethernet chip is chosen in
  `avr/nic.h` (`NIC_CHOICE`).

Build output is not kept in the repository; build it from source. The last firmware built in 2010 (2010-03-23) is attached
to the GitHub Release.

## History

[PROJECT.md](PROJECT.md) tells the story: how the history was recovered, the move from CVS to Subversion, the milestones
(*LOAD and *PEEK working across the bridge), what is lost, and the review of 2026.

- **`main`**: the CVS and Subversion history, oldest first, with the original dates and authors.
- **`Mark`**: Mark's work that was never committed: the assembler bridge's last working states (2009) and his C edits of
  the evening of 2010-01-23. It branches from the GNU indent commit on `main`.

## Credits

Phil Blundell and Mark Usher. The uIP TCP/IP stack is by Adam Dunkels (BSD licence; its own copyright notices are kept
in the files).

## Licence

GPL-3.0, see [LICENSE](LICENSE). Third-party parts (uIP, Atmel's `m162def.inc`, datasheets) keep their own terms.
