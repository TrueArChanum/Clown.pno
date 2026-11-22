
// clown_platform.h

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#ifndef CLOWN_PLATFORM_H
#define CLOWN_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>
#include "PNO_ABI.h"
#include "surface.h"

bool clown_platform_init(const KernelServices *services);
void clown_platform_shutdown(void);
void clown_platform_begin_frame(uint32_t delta_ticks);
void clown_platform_end_frame(void);
void clown_platform_handle_event(const PNO_Event *event);
uint32_t clown_platform_ticks(void);
Surface *clown_platform_surface(void);
bool clown_platform_key_down(uint32_t key);
void clown_platform_palette_set(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
void clown_platform_clear(uint8_t color);
bool clown_platform_audio_enabled(void);
bool clown_platform_audio_start(PNO_AudioPCMFillFn callback, void *user_data);
void clown_platform_audio_stop(void);
void clown_platform_log(uint32_t level, const char *message);
bool clown_platform_audio_set_frequency(uint32_t frequency);
void clown_platform_audio_tone_stop(void);

#endif
