# PROGRAM CHANGE -> SOUND, Digitone mk1

A synth track takes a sound from the project's pool, by MIDI program change
or from its own PRG knob, for the Digitone mk1 and Digitone Keys.

- **MIDI:** a program change on a synth track's channel (MIDI CONFIG >
  CHANNELS) loads that pool sound onto the track, as the SOUND BROWSER's
  FUNC + YES does: program 1 is pool sound 001, 128 is 128. Such a program
  change is the mod's: the stock does not also change the pattern for it.
  Other channels stay the stock's.
- **PRG** is knob A on TRIG's second page (press TRIG twice): -- or pool
  sound 001-128, a value per synth track in each pattern. Setting it loads
  that sound onto the track, and so does a pattern whose PRG differs from the
  last one's, as an Elektron MIDI track's PROG sends its program change when
  the pattern comes up.
- **P-lock PRG** on a trig (hold it in GRID REC and turn): that trig's note
  plays the pool sound at once, and the track keeps it from then on. A step
  with a stock sound lock keeps that.

The corner shows what was loaded for a second (T1 SOUND 005). A pool sound
past the pool's end loads nothing.

For OS 1.43 and 1.44, on [core-dn1](https://github.com/irpina/elekloader/tree/main/mods/core-dn1) 5.1. It names no firmware address: its
1.44 port is empty, and both builds carry the same code.

## Install

Not in elekloader's shop yet. You need:
- **core-dn1 5.1,** which is not released yet: build it from elekloader's source
  (`mods/core-dn1`, its README says how) with the stock OS file. Older cores refuse this mod.
- **elekloader from source with core-dn1 5.1** (its `digitone-mk1/core3.h` declares what this
  mod calls), with Python 3.9 or newer, to build the mod and the OS.
- **This mod:** `pcsound-0.2.elemod` for OS 1.43, `pcsound-0.2-os1.44.elemod` for 1.44, from
  [this repository's releases](https://github.com/irpina/digipcsound/releases) once there are any, or built
  from this folder (Build, below).
- **The stock OS file:** `Digitone_and_Digitone_Keys_OS1.44.syx` or `..._OS1.43.syx`, from
  [Elektron's Digitone downloads](https://www.elektron.se/support-downloads/digitone).
  elekloader recognises the file by its hash.

Build your OS (or in elekloader's window, run from source: **+ Install from file...** both files):

```bash
python -m elekloader.patch --stock Digitone_and_Digitone_Keys_OS1.44.syx \
    --mod core-5.1-os1.44.elemod --mod pcsound-0.2-os1.44.elemod \
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

The program change arrives in the MIDI task (`ev_midi_pc`); the load runs in
the UI task's next tick with `core_pool_load` (the pool's own getter and the
kit's load). PRG is a lane parameter (core-dn1 5.1's lane parameters (elekloader's docs/ADAPTING.md, "Lane parameters, knobs on pages and TRIG's second page")). The tick compares each
track's PRG value with the last one it loaded. A PRG p-lock takes effect
where the sequencer builds the note (`ev_note_built`): the note's sound
pointer is set to the pool sound, as a stock sound lock sets it, so that note
plays it with no delay. When the note's voice starts (`ev_voice_on`, with
`core_note_step` naming its step), the track is set to load that sound at the
next tick. Core rebuilds the first step PLAY plays when a lane changes while
stopped, so a lock set then plays too.

## Build

```bash
python -m elekloader.sdk.build . --stock Digitone_and_Digitone_Keys_OS1.43.syx   # out/pcsound-0.2.elemod
python -m elekloader.sdk.build . --stock Digitone_and_Digitone_Keys_OS1.44.syx   # out/pcsound-0.2-os1.44.elemod
python -m elekloader.lint out/pcsound-0.2.elemod --stock Digitone_and_Digitone_Keys_OS1.43.syx --with core-5.1.elemod
```

## What was checked

In digikit's emulator (digiemu, headless `Session`), with core-dn1 5.1, CHOKE
0.2 and a probe mod, on 1.43 and on 1.44:

- TRIG twice showed PRG on knob A beside CHK.
- T1's PRG value set to 005: the next tick loaded pool sound 005 onto T1
  (`ev_sound_load` saw it).
- T1's step 1 p-locked to 003 while stopped: the first note the sequencer
  built ahead now pointed at pool sound 003 (0x4135d9d6 on 1.43, 0x4135e9d6
  on 1.44) instead of the kit's; nothing loaded while editing; at PLAY the
  note played and the track loaded 003 at the next tick (the corner showed
  T1 SOUND 003).
- MIDI program changes: checked in 0.1 (ch1 to T1, ch2 to T2, ch5 ignored,
  program 128 loads), unchanged in 0.2.

`lint` says it patches no site and links with core-dn1 5.1 on 1.43 and 1.44.

Not checked: hearing the switched sound, a pattern change loading its PRG,
real MIDI input on DIN or USB (digiemu's UI task stops after injected MIDI
input, so 0.1 drove the program changes through `core_midi_receive`), the
Digitone Keys, and anything on hardware.

## Licence

GPL-2.0-or-later, as elekloader's core it builds on (LICENSE).
