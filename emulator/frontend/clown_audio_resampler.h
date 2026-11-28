
// clown_audio_resampler.h

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#ifndef CLOWN_AUDIO_RESAMPLER_H
#define CLOWN_AUDIO_RESAMPLER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "../emulator/clownmdemu-core/source/clownmdemu.h"  // 🤡

bool clown_audio_resampler_init(void);
void clown_audio_resampler_reset(void);

uint32_t clown_audio_resampler_fm(
    const cc_s16l *source,
    uint32_t source_frames,
    int16_t *output,
    uint32_t output_capacity);

uint32_t clown_audio_resampler_psg(
    const cc_s16l *source,
    uint32_t source_frames,
    int16_t *output,
    uint32_t output_capacity);

#endif
