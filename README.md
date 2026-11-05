# PROGRAM CHANGE -> SOUND, Digitone mk1

A program change on a synth track's MIDI channel loads that sound from the
project's pool onto the track, as the SOUND BROWSER's FUNC + YES does:
program 1 (0 on the wire) is pool sound 001, 128 is 128. For the Digitone
mk1 and Digitone Keys.

- The track channels are MIDI CONFIG > CHANNELS' (1-4 by default).
- Such a program change is the mod's: the stock does not also change the
  pattern for it, even when PROG CH IN is that channel.
- A program change on any other channel goes on to the stock as ever.
- The pool has 128 slots, so every program has one; what an empty slot loads (a blank sound, probably) was not checked.
- The screen's corner shows what was loaded (T1 SOUND 006) for a second.

For OS 1.43 and 1.44, on [core-dn1](https://github.com/irpina/elekloader/tree/main/mods/core-dn1) 5.0.1. It names no
firmware address: its 1.44 port is empty, and both builds carry the same
code.

## Install

Not in elekloader's shop yet. You need:
- **core-dn1 5.0.1,** which is not released yet: build it from elekloader's source
  (`mods/core-dn1`, its README says how) with the stock OS file. Older cores refuse this mod.
- **elekloader from source with core-dn1 5.0.1** (its `digitone-mk1/core3.h` declares what this
  mod calls), with Python 3.9 or newer, to build the mod and the OS.
- **This mod:** `pcsound-0.1.elemod` for OS 1.43, `pcsound-0.1-os1.44.elemod` for 1.44, from
  [this repository's releases](https://github.com/irpina/digipcsound/releases) once there are any, or built
  from this folder (Build, below).
- **The stock OS file:** `Digitone_and_Digitone_Keys_OS1.44.syx` or `..._OS1.43.syx`, from
  [Elektron's Digitone downloads](https://www.elektron.se/support-downloads/digitone).
  elekloader recognises the file by its hash.

Build your OS (or in elekloader's window, run from source: **+ Install from file...** both files):

```bash
python -m elekloader.patch --stock Digitone_and_Digitone_Keys_OS1.44.syx \
    --mod core-5.0.1-os1.44.elemod --mod pcsound-0.1-os1.44.elemod \
    --out Digitone_OS1.44-pcsound.syx --version XXXX
```

`--version` is the 4 characters the unit will show. Flash the `.syx` with
Elektron Transfer, as for any OS update: connect the unit over USB, select
it in Transfer and **Connect**, drag the `.syx` onto **Drop files here**,
and press **YES** on the unit. Don't turn it off until the upgrade is done.

**Recovery:** elekloader never changes the bootloader, so the stock OS
file always restores the unit. Hold **FUNC** while powering on for the
startup menu, and press **TRIG 4** for OS UPGRADE. Then send the stock
`.syx` with Transfer.

## How

`ev_midi_pc` (core 4.0) gives the mod each program change in the MIDI task,
before the stock applies it. If its channel is a synth track's
(`fw_midi_chan`, core 5.0.1), the mod takes it and notes the program for the
track; the UI task's next tick (30 a second) loads it with core 5.0.1's
`core_pool_load`, which does what the browser does: the pool's own getter
copies the sound, and the kit's load (0x4001d710, the routine
`ev_sound_load` hooks) puts it on the track. The load runs in the UI task,
where the browser runs it; the last program change for a track before a tick
wins.

`fw_sound_load` and `core_pool_load` went into core-dn1 rather than this mod:
the load is a kit method reached through a vtable, its kit and pool are
offsets into the project (0x250 and 0x1dcc8), and the pool's getter returns
its copy through a0 with a reference to release, none of which C can call
directly. In core, a mod (a sound randomiser, a sound sequencer) gets a
pool load from one call with no firmware address of its own.

## Build

```bash
python -m elekloader.sdk.build . --stock Digitone_and_Digitone_Keys_OS1.43.syx   # out/pcsound-0.1.elemod
python -m elekloader.sdk.build . --stock Digitone_and_Digitone_Keys_OS1.44.syx   # out/pcsound-0.1-os1.44.elemod
python -m elekloader.lint out/pcsound-0.1.elemod --stock Digitone_and_Digitone_Keys_OS1.43.syx --with core-5.0.1.elemod
```

## What was checked

In digikit's emulator (digiemu, headless `Session`), with core-dn1 5.0.1,
SOLO, LOOPBACK and a probe mod in the same build, on 1.43 and on 1.44:

- Program changes played into the unit with `core_midi_receive` (the MIDI
  task's own program change handler, as for one from DIN): program 6 on
  channel 1 loaded pool sound 006 onto T1 (`ev_sound_load` fired with track
  0, and the corner showed T1 SOUND 006); program 3 on channel 2 loaded onto
  T2; program 2 on channel 5, no track's, reached `ev_midi_pc` and loaded
  nothing; program 128 on channel 1 loaded slot 128.
- `core_pool_load` itself (core 5.0.1, on 1.43): pool sounds 1 and 3 change
  all of T1's 79 parameters and its algorithm; an index past 127 returns -1.
- A program change through the emulator's MIDI input (DIN, with INPUT FROM
  forced on) reached `pcs_pc`, which noted it for T1, on 1.43; the load did
  not follow there because digiemu's UI task stops after any injected MIDI
  message, with or without this mod (a CC does it too; the core-dn1 5.0 hook
  map notes the same stall after a device inquiry), so the load from real
  MIDI input was not seen in the emulator.

`lint` with core-dn1 5.0.1 (`--with core-5.0.1.elemod`) says it patches no site and links, on
1.43 and 1.44, with the same code on both; with core-dn1 5.0 it is refused: it needs 5.0.1.

Not checked: PROG CH IN set to a track's channel (the mod takes the change
before the stock, so the pattern should stay), the Digitone Keys, and
anything on hardware.

## Licence

GPL-2.0-or-later, as elekloader's core it builds on (LICENSE).
