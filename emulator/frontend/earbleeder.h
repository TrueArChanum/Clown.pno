
// earbleeder.h

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#ifndef EARBLEEDER_H
#define EARBLEEDER_H

#include <stddef.h>
#include <stdbool.h>

#include "../emulator/clownmdemu-core/source/clownmdemu.h"  // 🤡

void earbleeder_fm(
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_fm_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t));

void earbleeder_psg(
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_psg_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t));

void earbleeder_pcm(
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_pcm_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t));

void earbleeder_cdda(
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_cdda_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t));

//NOTE: different version than in speaker.c!
bool earbleeder_init(void);
void earbleeder_set_enabled(bool enabled);
void earbleeder_shutdown(void);

#endif
