# EconetBridge: project summary

How this repository came to be, what its history holds, and what is known about the code. Written in November 2026,
when the project moved from old copies on Mark Usher's computer into Git.

## Where the project lived

1. **2005-2009, Phil Blundell's design.** Drawings in 2005 (`docs/design/bridge1.pdf`, `bridge1a.pdf`), a first
   ATmega162 firmware in April 2009 (`asm/bridge.asm`, with Atmel's `m162def.inc` and the drawing `bridge_b.pdf`).
2. **June 2009, BerliOS.** The project `ecobridge` on BerliOS: a file release of `bridge.asm` (2009-06-22) and a CVS
   repository (`cvs.ecobridge.berlios.de:/cvsroot/ecobridge`, module `EconetBridge`).
3. **June-July 2009, the assembler bridge** (`EcEthBridge/`) and the experiments (`experiments/`).
4. **July 2009 to January 2010, the C bridge in CVS** (`avruip/`): the uIP TCP/IP stack on the AVR with the AUN
   application, mostly Mark's work on the IP side.
5. **March-April 2010, BerliOS Subversion.** The work moved to `svn.berlios.de/svnroot/repos/ecobridge/branch` (revisions
   r4 to r8, 2010-03-29 to 2010-04-05). That is the last state.
6. **2014.** BerliOS closed; the CVS and Subversion servers, and their history, are gone. SourceForge's archive of the
   project keeps only `bridge.asm`.

## How the history was recovered

There is no repository dump. The history was rebuilt from every surviving copy:
- the CVS working copies, whose `CVS/Entries` give each file's revision and the time CVS wrote it: a file whose time
  matches is the committed content of that revision;
- the dated backup folders, which are earlier working copies with their own `CVS/Entries`;
- the Subversion working copies, whose `.svn` text-base files hold each file's committed revision (checked by MD5);
- the pre-indent backups `NAME~` left by GNU indent, which match the committed revisions where those survive elsewhere;
- Phil Blundell's own folder, and the SourceForge copy of the published `bridge.asm`.

The result: **63 commit points** (6 dated files, 52 from CVS, 5 from Subversion) with **267 file revisions**, every one
checked by SHA-256, and no two copies disagreeing. Commit times come from CVS and Subversion; commit messages are lost, so
each commit lists the files and revisions it holds.

**Gaps.** 47 ranges of revisions are known only by number, because no copy of their content survives (mostly the early
CVS revisions 1.1 to 1.n of each file, `adlc_asm.S` 1.36, and the committed revisions of the assembler files in
`EcEthBridge`). Subversion r1 to r3 probably only created the folder layout.

**About `.#name.rev` files.** CVS writes these when an update merges other people's commits into a file you have changed:
`.#bridge.asm.1.12` is your *working file* as it was before the merge, on top of revision 1.12. They are not committed
revisions. The ones in `EcEthBridge/` are the only surviving content of those files, so they are on branch `Mark` as
working states; the ones in the C tree are superseded by later commits and are not in the history.

## Milestones

- **2010-01-23, *LOAD across the bridge works.** The backup "avruip 2010-01-23 load ok" is exactly the CVS state of that
  afternoon; that commit says so.
- **2010-01-23, GNU indent.** The C sources were reformatted (GNU style); its own commit, layout only.
- **2010-03-29, *PEEK across the bridge works.** The backup named "trunk 2010-01-29 peek ok" is really Subversion r6 of
  2010-03-29: immediate operations such as *PEEK were verified working across the bridge. That commit holds the fix.
- **January to March 2010** added IP fragmentation of large Econet packets (`avr/nic.c`), a destination-network check in
  the ADLC receive path (`drivers/adlc_asm.S`) and local-network handling in `apps/aun/aun.c`.

## The review of November 2026

The surviving code was read in full (not built or run). Findings:
- Mark's changes that were never committed to CVS (a tidy-up of names and headers on the evening of 2010-01-23) reached
  Subversion later in other form; the exact files are kept on branch `Mark`.
- The assembler bridge's last edits (2009-07) were never committed and are also on branch `Mark`.
- 27 bugs, listed in [KNOWN-ISSUES.md](KNOWN-ISSUES.md): 15 in the C bridge and 12 in the assembler bridge.

## Tooling

AVR Studio 4 project files (`.aps`, `.aws`) are kept next to the code they build. The C bridge also has Makefiles for
avr-gcc. Build output (`.o`, `.hex`, `.map`, listings) is not kept; the last firmware built in 2010 is on the Release.
