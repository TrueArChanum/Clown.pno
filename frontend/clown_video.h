
// clown_video.h

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#ifndef CLOWN_VIDEO_H
#define CLOWN_VIDEO_H

#include <stdint.h>

#include "../../surface.h"
#include "../emulator/clownmdemu-core/libraries/clowncommon/clowncommon.h"  // 🤡

void clown_video_init(Surface *surface);

void clown_video_colour_update(
    cc_u16f index,
    cc_u16f colour
);

void clown_video_scanline(
    Surface *surface,
    cc_u16f scanline,
    const cc_u8l *pixels,
    cc_u16f left,
    cc_u16f right,
    cc_u16f width,
    cc_u16f height);

#endif
