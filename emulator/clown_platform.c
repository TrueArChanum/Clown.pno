
// clown_platform.c

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#include "clown_platform.h"
#include <string.h>

typedef struct
{
    const KernelServices *services;
    PNO_Surface pno_surface;
    //Surface surface;
    bool keys[PNO_KEY_COUNT];
    uint32_t ticks;
    bool initialized;
} ClownPlatform;

static ClownPlatform platform;

bool clown_platform_init(const KernelServices *services)
{
    if (services == NULL || PNO_ABI_ValidateServices(services) != PNO_ABI_OK) return false;
    if ((services->capabilities & (PNO_CAP_MEMORY | PNO_CAP_VIDEO | PNO_CAP_VIDEO_PALETTE)) != (PNO_CAP_MEMORY | PNO_CAP_VIDEO | PNO_CAP_VIDEO_PALETTE)) return false;
    if (services->video_get_surface == NULL || services->video_clear == NULL || services->video_set_palette == NULL) return false;
    memset(&platform, 0, sizeof(platform));
    platform.services = services;
    platform.pno_surface.struct_size = sizeof(platform.pno_surface);
    if (services->video_get_surface(services->context, &platform.pno_surface) != 0) goto fail;
    if (platform.pno_surface.struct_size < sizeof(PNO_Surface) || platform.pno_surface.pixel_format != PNO_PIXEL_INDEX8 || platform.pno_surface.pixels == NULL || platform.pno_surface.width == 0u || platform.pno_surface.height == 0u || platform.pno_surface.pitch < platform.pno_surface.width) goto fail;
    /*
    platform.surface.pixels = platform.pno_surface.pixels;
    platform.surface.width = platform.pno_surface.width;
    platform.surface.height = platform.pno_surface.height;
    platform.surface.pitch = platform.pno_surface.pitch;
    */
    platform.initialized = true;
    return true;
    fail:
    memset(&platform, 0, sizeof(platform));
    return false;
}

void clown_platform_shutdown(void)
{
    memset(&platform, 0, sizeof(platform));
}

void clown_platform_begin_frame(uint32_t delta_ticks)
{
    if (platform.initialized) platform.ticks += delta_ticks;
}

void clown_platform_end_frame(void)
{
}

void clown_platform_handle_event(const PNO_Event *event)
{
    if (!platform.initialized || event == NULL || event->code >= PNO_KEY_COUNT) return;
    if (event->type == PNO_EVENT_KEY_DOWN) platform.keys[event->code] = true;
    else if (event->type == PNO_EVENT_KEY_UP) platform.keys[event->code] = false;
}

uint32_t clown_platform_ticks(void)
{
    if (!platform.initialized) return 0u;
    if (platform.services->get_ticks != NULL) return platform.services->get_ticks(platform.services->context);
    return platform.ticks;
}

PNO_Surface *clown_platform_surface(void)
{
    return platform.initialized ? &platform.pno_surface : NULL;
}

// Surface *clown_platform_surface(void)
// {
//     return platform.initialized ? &platform.surface : NULL;
// }

bool clown_platform_key_down(uint32_t key)
{
    return platform.initialized && key < PNO_KEY_COUNT && platform.keys[key];
}

void clown_platform_palette_set(uint8_t index, uint8_t red, uint8_t green, uint8_t blue)
{
    if (platform.initialized) platform.services->video_set_palette(platform.services->context, index, red, green, blue);
}

void clown_platform_clear(uint8_t color)
{
    if (platform.initialized) platform.services->video_clear(platform.services->context, color);
}

bool clown_platform_audio_enabled(void)
{
    if (!platform.initialized || platform.services->audio_get_enabled == NULL) return false;
    return platform.services->audio_get_enabled(platform.services->context) != 0;
}

bool clown_platform_audio_start(PNO_AudioPCMFillFn callback, void *user_data)
{
    if (!platform.initialized || callback == NULL || platform.services->audio_pcm_start == NULL) return false;
    return platform.services->audio_pcm_start(platform.services->context, callback, user_data) == 0;
}

void clown_platform_audio_stop(void)
{
    if (platform.initialized && platform.services->audio_pcm_stop != NULL) (void)platform.services->audio_pcm_stop(platform.services->context);
}

void clown_platform_log(uint32_t level, const char *message)
{
    if (platform.services != NULL && platform.services->log != NULL) platform.services->log(platform.services->context, level, message != NULL ? message : "");
}

bool clown_platform_audio_set_frequency(uint32_t frequency)
{
    if (!platform.initialized || frequency == 0u || platform.services->audio_tone_set_frequency == NULL) return false;
    return platform.services->audio_tone_set_frequency(platform.services->context, frequency) == 0;
}

void clown_platform_audio_tone_stop(void)
{
    if (platform.initialized && platform.services->audio_tone_stop != NULL)
        (void)platform.services->audio_tone_stop(platform.services->context);
}
