
// clown_audio.h

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#ifndef CLOWN_AUDIO_H
#define CLOWN_AUDIO_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "../emulator/clownmdemu-core/source/clownmdemu.h"  // 🤡

void clown_audio_mix_pending(void);

void clown_audio_psg(
    void *user_data,
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_psg_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t));

void clown_audio_fm(
    void *user_data,
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_fm_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t));

void clown_audio_pcm(
    void *user_data,
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_pcm_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t));

void clown_audio_cdda(
    void *user_data,
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_cdda_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t));


bool clown_sound_is_enabled(void);
void clown_sound_set_enabled(bool enabled);
void clown_sound_toggle(void);

void clown_audio_reset(void);
bool clown_audio_activate(void);
void clown_audio_deactivate(void);

#endif
