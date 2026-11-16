# Move record: EconetBridge

Read this first if you are a session picking up EconetBridge. It says where everything is and what changed when the
project moved into Git (November 2026).

## Where things are

| What | Where |
|---|---|
| The repository (public, GPL-3.0) | `github.com/mark-usher/EconetBridge` |
| `main` | the recovered CVS and Subversion history, 2005-2010, then tooling and documents |
| `Mark` | Mark Usher's work that was never committed: assembler-bridge working states (2009) and the C edits of 2010-01-23 evening; branches from the GNU indent commit |
| Local clone | `C:\Projects\acorn\EconetBridge` (Windows, AVR Studio) and `~/Projects/acorn/EconetBridge` (WSL, avr-gcc), from `proj sync` |
| The original folders | OneDrive `Current Projects\BBC\EconetBridge` and `Beeb\My Projects\EconetBridge`, renamed `_migrated_*` once checked; Phil Blundell's folder `Beeb\People\philblundel` stays (it holds his other projects) |
| Not in Git | the network captures and traces of 2009-2010 (`Logs\`), kept in OneDrive |

## Rules for sessions

- Work on a branch (`claude/<topic>` for agents), never on `main` or `Mark`; Mark merges.
- No AI mentions in commit messages or pull requests.
- The history on `main` is a record: don't rewrite it. New work goes on top.
- Fixes for [KNOWN-ISSUES.md](KNOWN-ISSUES.md) are new work: one branch per fix, with the issue number (C1, E2 ...).

## What happened

| Date | Change |
|---|---|
| 2026-11-16 | Inventory: BerliOS CVS and Subversion gone since 2014; copies in two OneDrive folders and Phil Blundell's folder; the published `bridge.asm` downloaded from the SourceForge archive |
| 2026-11-16 | Code review of every copy (local worker job `20261116-1854-econetbridge-review`): history map, uncommitted changes, 27 bugs |
| 2026-11-16 | History rebuilt from that map (change record CHG-0177), each revision checked by SHA-256; pushed to the public repository after Mark's go-ahead |
