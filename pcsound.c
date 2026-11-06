/* SPDX-License-Identifier: GPL-2.0-or-later */
/* PROGRAM CHANGE -> SOUND for the Digitone mk1 and Digitone Keys (core-dn1 5.1): a synth track takes a pool sound
 * from a program change it receives, or from its PRG knob on TRIG's second page.
 *
 * MIDI: a program change on a synth track's channel (MIDI CONFIG > CHANNELS, fw_midi_chan) loads pool sound
 * `program` (program 0, a controller's 1, is pool sound 001) onto that track, as the SOUND BROWSER's FUNC + YES
 * does: core_pool_load, the pool's own getter and the kit's load. The program change arrives in the MIDI task
 * (ev_midi_pc); the load runs in the UI task, where the browser runs it, at its next tick (30 a second): the last
 * program change for a track wins. A program change it loads a sound for is taken, so the stock does not change
 * the pattern for it as well (when PROG CH IN is that channel); one on any other channel goes on to the stock.
 *
 * PRG: knob A on TRIG's second page (core's, TRIG pressed twice), a core lane parameter: -- (none) or pool sound
 * 001-128. The track's value is the pattern's: setting it loads that sound onto the track, and so does a pattern
 * whose value differs from the last one's, as an Elektron MIDI track's PROG sends its program change when the
 * pattern starts. A p-lock (hold a trig in GRID REC and turn) switches the track's sound at that trig: the trig's
 * note plays it at once (ev_note_built puts the pool sound into the note, where a stock sound lock puts its own),
 * and the track keeps it from when that note starts, loaded at the next tick. A step with a stock sound lock keeps that.
 *
 * A pool sound past the pool's end loads nothing. The screen's corner shows what was loaded for a second. */
#include "digitone-mk1/core3.h"

typedef void (*rect_fn)(void *bmp, int x0, int y0, int x1, int y1, int colour);
typedef void (*text_fn)(void *bmp, const void *font, int x, int y, int maxlen, const char *fmt, ...);

#define PROG_ID         185             /* the parameter, a lane (param:185): 0 none, n pool sound n - 1 */

static volatile int32_t pending[4];     /* per track: the pool sound + 1, 0 none (MIDI task, interrupt) */
static int32_t last[4] = { -1, -1, -1, -1 };    /* the track values loaded last */
static volatile int32_t pool_count;     /* the UI task keeps it for the interrupt */
static int32_t shown, shown_track, shown_program, shown_result;
volatile int32_t pcs_switches;          /* p-locked notes switched, for the probe */

int32_t pcs_pc(int32_t channel, int32_t program, int32_t flags)
{
    int32_t took = 0;
    (void)flags;
    for (int32_t t = 0; t < 4; t++)
        if (fw_midi_chan[t] == channel) {
            pending[t] = program + 1;
            took = 1;
        }
    return took;
}

/* A note the sequencer builds (interrupt): a PRG p-lock on its step plays that pool sound. */
void pcs_built(int32_t track, int32_t step, void *event)
{
    int32_t p;
    const uint8_t *snd;
    if (track < 0 || track > 3)
        return;
    p = core_lane_lock(PROG_ID, track, step);
    if (p < 1 || p > pool_count)
        return;
    snd = CORE_EVENT_SOUND(event);
    if (snd >= fw_pool_sounds && snd < fw_pool_sounds + 128 * FW_POOL_SOUND_SIZE)
        return;                         /* a stock sound lock: the step's own */
    CORE_EVENT_SOUND(event) = fw_pool_sounds + (p - 1) * FW_POOL_SOUND_SIZE;
}

/* A voice starts (the render): a note from a step with a PRG p-lock switches its track to that sound for good,
 * loaded at the next tick. (Not at the build: the sequencer builds PLAY's first step ahead, while stopped.) */
void pcs_voice_on(int32_t voice, int32_t track, void *event)
{
    int32_t step, p;
    (void)voice;
    if (track < 0 || track > 3 || (step = core_note_step(event)) < 0)
        return;
    p = core_lane_lock(PROG_ID, track, step);
    if (p < 1 || p > pool_count || pending[track] == p)
        return;
    pending[track] = p;
    pcs_switches++;
}

void pcs_tick(void *ctrl)
{
    pool_count = core_pool_count();
    for (int32_t t = 0; t < 4; t++) {
        int32_t v = core_lane_track(PROG_ID, t), p;
        if (v != last[t]) {             /* set, or another pattern's */
            last[t] = v;
            if (v >= 1)
                pending[t] = v;
        }
        p = pending[t];
        if (!p)
            continue;
        pending[t] = 0;
        shown_result = core_pool_load(t, p - 1);
        shown_track = t;
        shown_program = p - 1;
        shown = 30;
    }
    if (shown > 0) {
        shown--;
        *((unsigned char *)ctrl + 0x20) = 1;
    }
}

void pcs_draw(void *bmp, void *ctrl)
{
    (void)ctrl;
    if (shown <= 0)
        return;
    ((rect_fn)fw_fillrect)(bmp, 76, 54, 127, 63, 0);
    ((text_fn)fw_textf)(bmp, (const void *)fw_font5, 79, 57, -1, shown_result == 0 ? "T%d SOUND %03d" : "T%d NO %03d",
                        (int)shown_track + 1, (int)shown_program + 1);
    ((rect_fn)fw_fillrect)(bmp, 76, 54, 127, 63, -1);
}

/* ---- the knob ------------------------------------------------------------------------------------ */
static void pcs_format(int32_t value, char *buf)
{
    int32_t v = value >> 8;
    if (v <= 0) {
        buf[0] = '-'; buf[1] = '-'; buf[2] = 0;
        return;
    }
    buf[0] = (char)('0' + v / 100);
    buf[1] = (char)('0' + v / 10 % 10);
    buf[2] = (char)('0' + v % 10);
    buf[3] = 0;
}

const struct core_param pcs_param = {
    PROG_ID, 16, CORE_SLOT_LANE, 0, 128 << 8, 0, 0, -1, -1, -1, 0,
    "Program", (const char *)fw_str_empty, "PRG", pcs_format, (const char *)fw_str_empty, 0, 0
};
const struct core_knob pcs_knob = { CORE_PAGE_TRIG2, 0, PROG_ID };
