
// earbleeder.c     🙉

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#include "earbleeder.h"
#include "../clown_platform.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define INVALID_AUDIO_SOURCE ((size_t)-1)

static size_t g_current_source = INVALID_AUDIO_SOURCE;
static size_t g_schedule_cursor;
static uint32_t g_source_frames_remaining;
static uint32_t g_current_frequency;
static uint32_t g_idle_grace_frames;
static bool g_enabled;

#define CLOWN_FM_SAMPLE_RATE_NTSC (CLOWNMDEMU_M68K_CLOCK_NTSC / FM_SAMPLE_RATE_DIVIDER)
#define CLOWN_FM_SAMPLE_RATE_PAL (CLOWNMDEMU_M68K_CLOCK_PAL / FM_SAMPLE_RATE_DIVIDER)

#define SPEAKER_MIN_FREQUENCY       16u
#define SPEAKER_MAX_FREQUENCY       11025u      // NOTE tested with 5000 too; all sound the same...

#define SPEAKER_IDLE_GRACE_FRAMES   64u         // global silence grace

#define PSG_SOURCE_COUNT CC_COUNT_OF(((ClownMDEmu *)0)->psg.state.tones)

#define TOTAL_AUDIO_SOURCES (PSG_SOURCE_COUNT + FM_TOTAL_CHANNELS)

// NOTE: consuming time twice: clown_audio_psg() and again from clown_audio_fm()
static const uint32_t g_source_weights[] =
{
    // MANUAL CHANNEL WEIGHTS TUNING: well, kinda a channel round-robin atm.

    // PSG
    100u, // PSG 0 semi important;  Sonic Jump SFX, sound & normal Background music; not used in Twinkle Tale. SF music.
    150u, // PSG 1 kinda important; Sonic Break SFX, normal background ; not used in Twinkle Tale. SF1 music.
     50u, // PSG 2 less important;  Does nothing in Sonic & Twinke Tale; but is used for Shining Force 1 high pitched "text beeps".
    100u, // PSG 3 semi important;  Sonic Intro SFX, normal background music & part of "Invicibility music"; "Beep Beep music" in Twinkle Tale. Music in SF1

    // FM
    150u, // FM 0 important, high pitch beep sounds, e.g. Twinkle Tale part of the background music & Sonic "drums?"
    150u, // FM 1 important for Sonic ("Invicibility music: DU-DU-DU DUDUDUUU"); Twinkle Tale part of the background music
    150u, // FM 2 important for Twinkle Tale & Sonic (Rings) but can "hang"; BEEEEEP; Earbleeder
    150u, // FM 3 important for Twinkle Tale Projectiles?
      0u, // FM 4 Does nothing!?
      0u  // FM 5 Does nothing!?
};

static uint32_t get_psg_sample_rate(const ClownMDEmu *clownmdemu)
{
    if (clownmdemu != NULL && clownmdemu->configuration.tv_standard == CLOWNMDEMU_TV_STANDARD_PAL)
    {
        return CLOWNMDEMU_PSG_SAMPLE_RATE_PAL;
    }

    return CLOWNMDEMU_PSG_SAMPLE_RATE_NTSC;
}

static uint32_t psg_tone_frequency(const ClownMDEmu *clownmdemu, size_t channel)
{
    const PSG_ToneState *tone;
    uint32_t sample_rate;

    if (clownmdemu == NULL)                                     return 0;
    if (channel >= CC_COUNT_OF(clownmdemu->psg.state.tones))    return 0;

    tone = &clownmdemu->psg.state.tones[channel];

    if (tone->countdown_master == 0)                            return 0;

    sample_rate = get_psg_sample_rate(clownmdemu);

    return sample_rate / (2u * (uint32_t)tone->countdown_master);
}

static uint32_t get_fm_sample_rate(const ClownMDEmu *clownmdemu)
{
    if (clownmdemu != NULL && clownmdemu->configuration.tv_standard == CLOWNMDEMU_TV_STANDARD_PAL)
    {
        return CLOWN_FM_SAMPLE_RATE_PAL;
    }

    return CLOWN_FM_SAMPLE_RATE_NTSC;
}

static uint32_t fm_channel_frequency(const FM_Channel *channel, uint32_t sample_rate)
{
    uint32_t f_number;
    uint32_t block;
    uint32_t shift;
    uint32_t whole;
    uint32_t remainder;
    uint32_t result;

    if (channel == NULL)    return 0;

    f_number = channel->operators[0].phase.f_number_and_block & 0x7FFu;

    block = (channel->operators[0].phase.f_number_and_block >> 11) & 7u;

    if (f_number == 0)      return 0;

    shift = 21u - block;

    whole = sample_rate >> shift;
    remainder = sample_rate & ((1u << shift) - 1u);

    result = f_number * whole;
    result += (f_number * remainder) >> shift;

    return result;
}

static bool frequency_is_usable(uint32_t frequency)
{
    if (frequency < SPEAKER_MIN_FREQUENCY)  return false;
    if (frequency > SPEAKER_MAX_FREQUENCY)  return false;

    return true;
}

static bool psg_source_active(const ClownMDEmu *clownmdemu, size_t channel)
{
    uint32_t frequency;

    if (clownmdemu == NULL) return false;
    if (channel >= PSG_SOURCE_COUNT) return false;
    if (clownmdemu->psg.configuration.tone_disabled[channel]) return false;
    if (clownmdemu->psg.state.tones[channel].attenuation >= 15) return false;

    frequency = psg_tone_frequency(clownmdemu, channel);

    return frequency_is_usable(frequency);
}

static bool fm_source_active(const ClownMDEmu *clownmdemu, size_t channel_index)
{
    const FM_Channel *channel;

    if (clownmdemu == NULL)                                                 return false;
    if (channel_index >= FM_TOTAL_CHANNELS)                                 return false;
    if (channel_index == 5 && clownmdemu->fm.state.dac_enabled)             return false;
    if (clownmdemu->fm.configuration.fm_channels_disabled[channel_index])   return false;

    channel = &clownmdemu->fm.state.channels[channel_index].state;

    for (size_t operator_index = 0; operator_index < FM_TOTAL_OPERATORS; ++operator_index)
    {
        if (channel->operators[operator_index].key_on)                      return true;
    }
    return false;
}

static uint32_t source_frequency(const ClownMDEmu *clownmdemu, size_t source_index)
{
    uint32_t frequency;

    if (clownmdemu == NULL)                                 return 0;

    if (source_index < PSG_SOURCE_COUNT)
    {
        if (!psg_source_active(clownmdemu, source_index))   return 0;

        frequency = psg_tone_frequency(clownmdemu, source_index);

        if (!frequency_is_usable(frequency))                return 0;

        return frequency;
    }

    source_index -= PSG_SOURCE_COUNT;

    if (source_index >= FM_TOTAL_CHANNELS)                  return 0;
    if (!fm_source_active(clownmdemu, source_index))        return 0;


    frequency = fm_channel_frequency(&clownmdemu->fm.state.channels[source_index].state, get_fm_sample_rate(clownmdemu));

    if (!frequency_is_usable(frequency))                    return 0;

    return frequency;
}

static size_t find_next_source(const ClownMDEmu *clownmdemu)
{
    if (clownmdemu == NULL)         return TOTAL_AUDIO_SOURCES;
    if (TOTAL_AUDIO_SOURCES == 0)   return TOTAL_AUDIO_SOURCES;

    for (size_t offset = 0; offset < TOTAL_AUDIO_SOURCES; ++offset)
    {
        size_t source_index = g_schedule_cursor + offset;

        if (source_index >= TOTAL_AUDIO_SOURCES)    source_index -= TOTAL_AUDIO_SOURCES;

        if (g_source_weights[source_index] == 0)                continue;
        if (source_frequency(clownmdemu, source_index) == 0)    continue;

        g_schedule_cursor = source_index + 1u;

        if (g_schedule_cursor >= TOTAL_AUDIO_SOURCES)   g_schedule_cursor = 0;

        return source_index;
    }

    return TOTAL_AUDIO_SOURCES;
}

static void set_frequency(uint32_t frequency)
{
    if (!frequency_is_usable(frequency) || frequency == g_current_frequency)    return;
    if (!clown_platform_audio_set_frequency(frequency))                         return;
    g_current_frequency = frequency;
}

static void stop(void)
{
    clown_platform_audio_tone_stop();
    g_current_frequency = 0u;
}

//Lifecycle functions (replacement for speaker.c)
bool earbleeder_init(void)
{
    g_enabled = true;
    g_current_source = INVALID_AUDIO_SOURCE;
    g_schedule_cursor = 0u;
    g_source_frames_remaining = 0u;
    g_current_frequency = 0u;
    g_idle_grace_frames = 0u;
    clown_platform_audio_tone_stop();
    return true;
}

void earbleeder_set_enabled(bool enabled)
{
    g_enabled = enabled;

    if (!enabled)
    {
        stop();
        g_current_source = INVALID_AUDIO_SOURCE;
        g_schedule_cursor = 0u;
        g_source_frames_remaining = 0u;
        g_idle_grace_frames = 0u;
    }
}

void earbleeder_shutdown(void)
{
    earbleeder_set_enabled(false);
}

static bool start_next_source(ClownMDEmu *clownmdemu)
{
    size_t source_index;

    if (clownmdemu == NULL) return false;

    source_index = find_next_source(clownmdemu);

    if (source_index >= TOTAL_AUDIO_SOURCES)
    {
        g_current_source = INVALID_AUDIO_SOURCE;
        g_source_frames_remaining = 0;

        return false;
    }

    g_current_source = source_index;

    g_source_frames_remaining = g_source_weights[source_index];

    if (g_source_frames_remaining == 0)
    {
        g_current_source = INVALID_AUDIO_SOURCE;
        return false;
    }

    return true;
}

static void arbitrate(ClownMDEmu *clownmdemu, size_t total_frames)
{
    if (clownmdemu == NULL)     return;
    if (total_frames == 0)      return;

    while (total_frames != 0)
    {
        if (g_source_frames_remaining == 0 || g_current_source == INVALID_AUDIO_SOURCE)
        {
            if (!start_next_source(clownmdemu))
            {
                if (g_idle_grace_frames < SPEAKER_IDLE_GRACE_FRAMES)
                {
                    uint32_t remaining_grace = SPEAKER_IDLE_GRACE_FRAMES - g_idle_grace_frames;

                    if (remaining_grace > (uint32_t)total_frames)
                    {
                        g_idle_grace_frames += (uint32_t)total_frames;
                    }
                    else
                    {
                        g_idle_grace_frames = SPEAKER_IDLE_GRACE_FRAMES;
                        stop();
                    }
                }

                return;
            }
        }

        g_idle_grace_frames = 0;

        uint32_t frequency = source_frequency(clownmdemu, g_current_source);

        if (frequency == 0)
        {
            g_current_source = INVALID_AUDIO_SOURCE;
            g_source_frames_remaining = 0;

            continue;
        }

        set_frequency(frequency);

        uint32_t frames = g_source_frames_remaining;

        if (frames > (uint32_t)total_frames)    frames = (uint32_t)total_frames;

        g_source_frames_remaining -= frames;
        total_frames -= frames;

        if (g_source_frames_remaining == 0) g_current_source = INVALID_AUDIO_SOURCE;
    }
}

void earbleeder_fm(
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_fm_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t))
{
    (void)generate_fm_audio;

    if (!g_enabled) return;

    arbitrate(clownmdemu, total_frames);
}

void earbleeder_psg(
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_psg_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t))
{
    (void)generate_psg_audio;

    if (!g_enabled) return;

    arbitrate(clownmdemu, total_frames);
}

// NOTE: Mega-CD; Not used by this project.
void earbleeder_pcm(
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_pcm_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t))
{
    (void)clownmdemu;
    (void)total_frames;
    (void)generate_pcm_audio;
}

void earbleeder_cdda(
    ClownMDEmu *clownmdemu,
    size_t total_frames,
    void (*generate_cdda_audio)(
    ClownMDEmu *,
    cc_s16l *,
    size_t))
{
    (void)clownmdemu;
    (void)total_frames;
    (void)generate_cdda_audio;
}
