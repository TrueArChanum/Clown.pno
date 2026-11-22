
// clown_audio_resampler.c

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#include "clown_audio_resampler.h"

#include "../emulator/clownmdemu-core/libraries/clownresampler/clownresampler.h"  // 🤡
#include "../../HD_audio.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define CLOWN_RESAMPLER_OUTPUT_RATE HDA_PCM_SAMPLE_RATE_48000
#define CLOWN_RESAMPLER_FM_RATE CLOWNMDEMU_FM_SAMPLE_RATE_NTSC      //  53267u
#define CLOWN_RESAMPLER_PSG_RATE CLOWNMDEMU_PSG_SAMPLE_RATE_NTSC    //  223722u <- 223721.5625
#define CLOWN_RESAMPLER_LOW_PASS_RATE 44100u

#define CLOWN_RESAMPLER_FM_CHANNELS  2u
#define CLOWN_RESAMPLER_PSG_CHANNELS 1u

typedef struct
{
    const cc_s16l *source;
    size_t source_frames;
    size_t source_position;
    uint8_t channels;
    bool mono;

    int16_t *output;
    uint32_t output_capacity;
    uint32_t output_frames;

} CLOWN_RESAMPLER_CALLBACK_DATA;

static ClownResampler_Precomputed g_resampler_precomputed;

static ClownResampler_HighLevel_State g_fm_resampler;
static ClownResampler_HighLevel_State g_psg_resampler;

static bool g_resampler_initialized;

static int16_t clown_resampler_clamp16(cc_s32f value)
{
    if (value > 32767)
        return 32767;

    if (value < -32768)
        return -32768;

    return (int16_t)value;
}

static size_t clown_resampler_input_callback(void *user_data, cc_s16l *buffer, size_t total_frames)
{
    CLOWN_RESAMPLER_CALLBACK_DATA *data;
    size_t remaining;
    size_t frames;
    size_t i;

    data = (CLOWN_RESAMPLER_CALLBACK_DATA *)user_data;

    if (data == NULL || buffer == NULL) return 0;

    remaining = data->source_frames - data->source_position;

    frames = total_frames;

    if (frames > remaining)
        frames = remaining;

    for (i = 0; i < frames; ++i)
    {
        size_t source_index;

        source_index = data->source_position + i;

        if (data->mono)
        {
            buffer[i] = data->source[source_index];
        }
        else
        {
            buffer[i * 2u] = data->source[source_index * 2u];
            buffer[i * 2u + 1u] = data->source[source_index * 2u + 1u];
        }
    }

    data->source_position += frames;

    return frames;
}

static cc_bool clown_resampler_output_callback(void *user_data, const cc_s32f *frame, cc_u8f total_samples)
{
    CLOWN_RESAMPLER_CALLBACK_DATA *data;
    uint32_t index;

    data = (CLOWN_RESAMPLER_CALLBACK_DATA *)user_data;

    if (data == NULL || frame == NULL)                  return cc_false;
    if (data->output_frames >= data->output_capacity)   return cc_false;
    if (total_samples != data->channels)                return cc_false;

    index = data->output_frames * 2u;

    if (data->mono)
    {
        data->output[index] = clown_resampler_clamp16(frame[0]);
        data->output[index + 1u] = data->output[index];
    }
    else
    {
        data->output[index] = clown_resampler_clamp16(frame[0]);
        data->output[index + 1u] = clown_resampler_clamp16(frame[1]);
    }

    data->output_frames++;

    return data->output_frames < data->output_capacity ? cc_true : cc_false;
}

bool clown_audio_resampler_init(void)
{
    if (g_resampler_initialized)
        return true;

    ClownResampler_Precompute(&g_resampler_precomputed);

    if (!ClownResampler_HighLevel_Init(
        &g_fm_resampler,
        CLOWN_RESAMPLER_FM_CHANNELS,
        CLOWN_RESAMPLER_FM_RATE,
        CLOWN_RESAMPLER_OUTPUT_RATE,
        CLOWN_RESAMPLER_LOW_PASS_RATE))
    {
        return false;
    }

    if (!ClownResampler_HighLevel_Init(
        &g_psg_resampler,
        CLOWN_RESAMPLER_PSG_CHANNELS,
        CLOWN_RESAMPLER_PSG_RATE,
        CLOWN_RESAMPLER_OUTPUT_RATE,
        CLOWN_RESAMPLER_LOW_PASS_RATE))
    {
        return false;
    }

    g_resampler_initialized = true;

    return true;
}

void clown_audio_resampler_reset(void)
{
    if (!g_resampler_initialized)
    {
        clown_audio_resampler_init();
        return;
    }

    ClownResampler_HighLevel_Reset(&g_fm_resampler);
    ClownResampler_HighLevel_Reset(&g_psg_resampler);
}

static uint32_t clown_audio_resampler_process(
    ClownResampler_HighLevel_State *resampler,
    const cc_s16l *source,
    uint32_t source_frames,
    int16_t *output,
    uint32_t output_capacity,
    uint8_t channels,
    bool mono)
{
    CLOWN_RESAMPLER_CALLBACK_DATA data;

    if (!g_resampler_initialized)                   return 0;
    if (source == NULL || output == NULL)           return 0;
    if (source_frames == 0 || output_capacity == 0) return 0;

    data.source = source;
    data.source_frames = source_frames;
    data.source_position = 0;
    data.channels = channels;
    data.mono = mono;

    data.output = output;
    data.output_capacity = output_capacity;
    data.output_frames = 0;

    ClownResampler_HighLevel_Resample(
        resampler,
        &g_resampler_precomputed,
        clown_resampler_input_callback,
        clown_resampler_output_callback,
        &data);

    return data.output_frames;
}

uint32_t clown_audio_resampler_fm(
    const cc_s16l *source,
    uint32_t source_frames,
    int16_t *output,
    uint32_t output_capacity)
{
    return clown_audio_resampler_process(
        &g_fm_resampler,
        source,
        source_frames,
        output,
        output_capacity,
        CLOWN_RESAMPLER_FM_CHANNELS,
        false);
}

uint32_t clown_audio_resampler_psg(
    const cc_s16l *source,
    uint32_t source_frames,
    int16_t *output,
    uint32_t output_capacity)
{
    return clown_audio_resampler_process(
        &g_psg_resampler,
        source,
        source_frames,
        output,
        output_capacity,
        CLOWN_RESAMPLER_PSG_CHANNELS,
        true);
}
