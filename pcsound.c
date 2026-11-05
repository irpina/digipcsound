/* SPDX-License-Identifier: GPL-2.0-or-later */
/* PROGRAM CHANGE -> SOUND for the Digitone mk1 and Digitone Keys (core-dn1 5.0.1). A program change on a synth
 * track's channel (MIDI CONFIG > CHANNELS, fw_midi_chan) loads pool sound `program` (program 0, a controller's
 * 1, is pool sound 001) onto that track, as the SOUND BROWSER's FUNC + YES does: core_pool_load, the pool's own
 * getter and the kit's load. The program change arrives in the MIDI task (ev_midi_pc); the load runs in the UI
 * task, where the browser runs it, at its next tick (30 a second): the last program change for a track wins.
 * A program change it loads a sound for is taken, so the stock does not change the pattern for it as well (when
 * PROG CH IN is that channel); one on any other channel goes on to the stock. A program change past the pool's
 * end loads nothing. The screen's corner shows what was loaded for a second. */
#include "digitone-mk1/core3.h"

typedef void (*rect_fn)(void *bmp, int x0, int y0, int x1, int y1, int colour);
typedef void (*text_fn)(void *bmp, const void *font, int x, int y, int maxlen, const char *fmt, ...);

static volatile int32_t pending[4];      /* per track: the program + 1, 0 none (the MIDI task writes) */
static int32_t shown, shown_track, shown_program, shown_result;

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

void pcs_tick(void *ctrl)
{
    for (int32_t t = 0; t < 4; t++) {
        int32_t p = pending[t];
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
