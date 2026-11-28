
// clown_audio.c

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#include <stdbool.h>

#include "clown_audio.h"
#include "clown_audio_resampler.h"
#include "earbleeder.h"

#include "clown_platform.h"


//BEGIN PSG/FM software mixer
#define CLOWN_AUDIO_OUTPUT_RATE       48000u

#define CLOWN_AUDIO_TEMP_FRAMES       1024u
#define CLOWN_AUDIO_PENDING_FRAMES    4096u
#define CLOWN_AUDIO_FIFO_FRAMES       4096u

// NTSC: 7,670,454 / 144 = 53,267.04 Hz   &   PAL: 7,600,489 / 144 = 52,781.17 Hz
#define CLOWN_AUDIO_FM_RATE CLOWNMDEMU_FM_SAMPLE_RATE_NTSC
#define CLOWN_AUDIO_PSG_RATE CLOWNMDEMU_PSG_SAMPLE_RATE_NTSC


typedef struct
{
    int32_t left[CLOWN_AUDIO_TEMP_FRAMES];
    int32_t right[CLOWN_AUDIO_TEMP_FRAMES];

} CLOWN_AUDIO_MIXER;


typedef struct
{
    int16_t samples[CLOWN_AUDIO_FIFO_FRAMES * 2u];

    uint32_t read_frame;
    uint32_t write_frame;
    uint32_t count;

} CLOWN_AUDIO_FIFO;


static CLOWN_AUDIO_FIFO g_audio_fifo;

static CLOWN_AUDIO_MIXER g_audio_mixer;

static bool g_sound_enabled;


typedef enum
{
    CLOWN_AUDIO_BACKEND_NONE,
    CLOWN_AUDIO_BACKEND_EARBLEEDER,
    CLOWN_AUDIO_BACKEND_HDA

} ClownAudioBackendType;


static ClownAudioBackendType g_backend;

static cc_s16l g_psg_buffer[CLOWN_AUDIO_TEMP_FRAMES * 2u];
static cc_s16l g_fm_buffer[CLOWN_AUDIO_TEMP_FRAMES * 2u];


static int16_t clown_audio_clamp16(
    int32_t value)
{
    if (value > 32767)
        return 32767;

    if (value < -32768)
        return -32768;

    return (int16_t)value;
}

static void clown_audio_fifo_reset(void)
{
    g_audio_fifo.read_frame = 0;
    g_audio_fifo.write_frame = 0;
    g_audio_fifo.count = 0;
}

static uint32_t clown_audio_fifo_write(
    const int16_t *samples,
    uint32_t frames)
{
    uint32_t written;

    if (samples == 0)   return 0;

    written = 0;

    while (written < frames)
    {
        uint32_t index;

        if (g_audio_fifo.count >= CLOWN_AUDIO_FIFO_FRAMES)  break;

        index = g_audio_fifo.write_frame;

        g_audio_fifo.samples[index * 2u] = samples[written * 2u];

        g_audio_fifo.samples[index * 2u + 1u] = samples[written * 2u + 1u];

        g_audio_fifo.write_frame++;

        if (g_audio_fifo.write_frame >= CLOWN_AUDIO_FIFO_FRAMES)
        {
            g_audio_fifo.write_frame = 0;
        }

        g_audio_fifo.count++;
        written++;
    }

    return written;
}

static uint32_t clown_audio_fifo_read(int16_t *samples, uint32_t frames)
{
    uint32_t read;

    if (samples == 0)   return 0;

    read = 0;

    while (read < frames)
    {
        uint32_t index;

        if (g_audio_fifo.count == 0)    break;

        index = g_audio_fifo.read_frame;

        samples[read * 2u] = g_audio_fifo.samples[index * 2u];
        samples[read * 2u + 1u] = g_audio_fifo.samples[index * 2u + 1u];

        g_audio_fifo.read_frame++;

        if (g_audio_fifo.read_frame >= CLOWN_AUDIO_FIFO_FRAMES)
        {
            g_audio_fifo.read_frame = 0;
        }

        g_audio_fifo.count--;
        read++;
    }

    return read;
}

static void clown_audio_mixer_clear(uint32_t frames)
{
    if (frames > CLOWN_AUDIO_TEMP_FRAMES)
        frames = CLOWN_AUDIO_TEMP_FRAMES;

    for (uint32_t i = 0; i < frames; ++i)
    {
        g_audio_mixer.left[i] = 0;
        g_audio_mixer.right[i] = 0;
    }
}

static void clown_audio_mixer_output(uint32_t frames)
{
    static int16_t output[CLOWN_AUDIO_TEMP_FRAMES * 2u];

        if (frames == 0)    return;

    if (frames > CLOWN_AUDIO_TEMP_FRAMES)
        frames = CLOWN_AUDIO_TEMP_FRAMES;

    for (uint32_t i = 0; i < frames; ++i)
    {
        output[i * 2u] = clown_audio_clamp16(g_audio_mixer.left[i]);

        output[i * 2u + 1u] = clown_audio_clamp16(g_audio_mixer.right[i]);
    }

    clown_audio_fifo_write(output, frames);
}

static void clown_hda_fill(void *user_data, int16_t *samples, uint32_t frames)
{
    (void)user_data;

    if (samples == 0)   return;

    if (!g_sound_enabled)
    {
        for (uint32_t i = 0; i < frames; ++i)
             {
                 samples[i * 2u] = 0;
                 samples[i * 2u + 1u] = 0;
             }

             return;
    }

    uint32_t read;

    read = clown_audio_fifo_read(samples, frames);

    while (read < frames)
    {
        samples[read * 2u] = 0;
        samples[read * 2u + 1u] = 0;
        read++;
    }
}
//END mixer

//BEGIN resampler
//NOTE: using clownresampler.h now!

static int16_t g_fm_pending[CLOWN_AUDIO_PENDING_FRAMES * 2u];
static int16_t g_psg_pending[CLOWN_AUDIO_PENDING_FRAMES * 2u];

static uint32_t g_fm_pending_frames;
static uint32_t g_psg_pending_frames;

void clown_audio_mix_pending(void)
{
    uint32_t frames;
    uint32_t fm_frames;
    uint32_t psg_frames;

    fm_frames = g_fm_pending_frames;
    psg_frames = g_psg_pending_frames;

    frames = fm_frames;

    if (psg_frames < frames)
        frames = psg_frames;

    if (frames > CLOWN_AUDIO_TEMP_FRAMES)
        frames = CLOWN_AUDIO_TEMP_FRAMES;

    if (frames == 0)    return;

    clown_audio_mixer_clear(frames);

    for (uint32_t i = 0; i < frames; ++i)
    {
        g_audio_mixer.left[i] = (int32_t)g_fm_pending[i * 2u] + (int32_t)g_psg_pending[i * 2u];
        g_audio_mixer.right[i] = (int32_t)g_fm_pending[i * 2u + 1u] + (int32_t)g_psg_pending[i * 2u + 1u];
    }

    clown_audio_mixer_output(frames);

    g_fm_pending_frames -= frames;
    g_psg_pending_frames -= frames;

    if (g_fm_pending_frames != 0)
    {
        for (uint32_t i = 0; i < g_fm_pending_frames; ++i)
             {
                 g_fm_pending[i * 2u] = g_fm_pending[(i + frames) * 2u];
                 g_fm_pending[i * 2u + 1u] = g_fm_pending[(i + frames) * 2u + 1u];
             }
    }

    if (g_psg_pending_frames != 0)
    {
        for (uint32_t i = 0; i < g_psg_pending_frames; ++i)
             {
                 g_psg_pending[i * 2u] = g_psg_pending[(i + frames) * 2u];
                 g_psg_pending[i * 2u + 1u] = g_psg_pending[(i + frames) * 2u + 1u];
             }
    }
}
//END resampler

//BEGIN Dispatch callbacks

void clown_audio_psg(
    void *user_data,
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_psg_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t))
{
    (void)user_data;

    if (!g_sound_enabled)   return;

    switch (g_backend)
    {
        case CLOWN_AUDIO_BACKEND_HDA:
        {
            size_t offset;

            if (clownmdemu == 0)            return;
            if (generate_psg_audio == 0)    return;

            offset = 0;

            while (offset < total_frames)
            {
                uint32_t frames;
                uint32_t remaining;
                uint32_t output_frames;

                frames = (uint32_t)(total_frames - offset);

                if (frames > CLOWN_AUDIO_TEMP_FRAMES)
                    frames = CLOWN_AUDIO_TEMP_FRAMES;

                for (uint32_t i = 0; i < frames; ++i)
                    g_psg_buffer[i] = 0;

                     generate_psg_audio(
                         clownmdemu,
                         g_psg_buffer,
                         frames);

                     remaining = CLOWN_AUDIO_PENDING_FRAMES - g_psg_pending_frames;

                     if (remaining != 0)
                     {

                         output_frames =
                         clown_audio_resampler_psg(
                             g_psg_buffer,
                             frames,
                             &g_psg_pending[
                             g_psg_pending_frames * 2u],
                             remaining);

                         g_psg_pending_frames += output_frames;
                     }

                     offset += frames;
            }
        }
        break;

        case CLOWN_AUDIO_BACKEND_EARBLEEDER:

            earbleeder_psg(
                clownmdemu,
                total_frames,
                generate_psg_audio);

            break;

        default:
            break;
    }
}

void clown_audio_fm(
    void *user_data,
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_fm_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t))
{
    (void)user_data;

    if (!g_sound_enabled) return;

    switch (g_backend)
    {
        case CLOWN_AUDIO_BACKEND_HDA:
        {
            size_t offset;

            if (clownmdemu == 0)            return;
            if (generate_fm_audio == 0)     return;

            offset = 0;

            while (offset < total_frames)
            {
                uint32_t frames;
                uint32_t remaining;
                uint32_t output_frames;

                frames = (uint32_t)(total_frames - offset);

                if (frames > CLOWN_AUDIO_TEMP_FRAMES)
                    frames = CLOWN_AUDIO_TEMP_FRAMES;

                for (uint32_t i = 0; i < frames * 2u; ++i)
                     {
                         g_fm_buffer[i] = 0;
                     }

                     generate_fm_audio(
                         clownmdemu,
                         g_fm_buffer,
                         frames);

                     remaining = CLOWN_AUDIO_PENDING_FRAMES - g_fm_pending_frames;

                     if (remaining != 0)
                     {
                         output_frames =
                         clown_audio_resampler_fm(
                             g_fm_buffer,
                             frames,
                             &g_fm_pending[
                             g_fm_pending_frames * 2u],
                             remaining);

                         g_fm_pending_frames += output_frames;
                     }

                     offset += frames;
            }
        }
        break;

        case CLOWN_AUDIO_BACKEND_EARBLEEDER:

            earbleeder_fm(
                clownmdemu,
                total_frames,
                generate_fm_audio);

            break;

        default:
            break;
    }
}

// NOTE: Mega-CD not supported (yet...)
void clown_audio_pcm(
    void *user_data,
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_pcm_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t))
{
    (void)user_data;
    (void)clownmdemu;
    (void)total_frames;
    (void)generate_pcm_audio;
}

// NOTE: Mega-CD not supported
void clown_audio_cdda(
    void *user_data,
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_cdda_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t))
{
    (void)user_data;
    (void)clownmdemu;
    (void)total_frames;
    (void)generate_cdda_audio;

}
//END Dispatch callbacks

//BEGIN Sound API
bool clown_sound_is_enabled(void)
{
    return g_sound_enabled;
}

void clown_sound_set_enabled(bool enabled)
{
    g_sound_enabled = enabled;

    if (g_backend == CLOWN_AUDIO_BACKEND_EARBLEEDER)
    {
        earbleeder_set_enabled(enabled);
    }
}

void clown_sound_toggle(void)
{
    clown_sound_set_enabled(
        !g_sound_enabled);
}

// Ownership stuff
void clown_audio_reset(void)
{
    g_sound_enabled = false;
    g_backend = CLOWN_AUDIO_BACKEND_NONE;

    clown_audio_fifo_reset();

    g_fm_pending_frames = 0;
    g_psg_pending_frames = 0;

    clown_audio_resampler_reset();
}

bool clown_audio_activate(void)
{
    bool enabled = clown_platform_audio_enabled();

    if (!clown_audio_resampler_init()) return false;
    clown_audio_reset();

    if (clown_platform_audio_start(clown_hda_fill, NULL))
    {
        g_backend = CLOWN_AUDIO_BACKEND_HDA;
        g_sound_enabled = enabled;
        return true;
    }
    // 🙉 Fall back to the PC-speaker Earbleeder backend.
    if (earbleeder_init())
    {
        g_backend = CLOWN_AUDIO_BACKEND_EARBLEEDER;
        g_sound_enabled = enabled;
        earbleeder_set_enabled(enabled);
        return true;
    }

    return false;
}

void clown_audio_deactivate(void)
{
    if (g_backend == CLOWN_AUDIO_BACKEND_HDA)
        clown_platform_audio_stop();
    else if (g_backend == CLOWN_AUDIO_BACKEND_EARBLEEDER)
        earbleeder_shutdown();

    clown_platform_audio_tone_stop();
    clown_audio_reset();
}
//END Sound API
