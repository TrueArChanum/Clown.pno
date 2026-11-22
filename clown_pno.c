
// clown_pno.c

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "PNO_ABI.h"
#include "clown_platform.h"
#include "clown_audio.h"
#include "clownmdemu.h"

#define CLOWN_SAVE_OBJECT_NAME      "GenesisData"
#define CLOWN_ROM_MAX_BYTES         (6u * 1024u * 1024u)
#define CLOWN_SRAM_MAX_BYTES        65536u
#define CLOWN_SAVE_HEADER_BYTES     16u
#define CLOWN_SAVE_OBJECT_BYTES     (CLOWN_SAVE_HEADER_BYTES + CLOWN_SRAM_MAX_BYTES)
#define CLOWN_SAVE_MAGIC            0x47454E53u
#define CLOWN_SAVE_VERSION          1u


typedef struct
{
    const KernelServices *services;
    ClownMDEmu *emu;
    ClownMDEmu_Callbacks callbacks;
    cc_u16l *rom;
    uint32_t rom_bytes;
    uint32_t rom_words;
    uint32_t rom_object_id;
    bool audio_started;
    bool initialized;
} ClownPNOState;


static uint32_t clown_read_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void clown_write_u32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static uint32_t clown_checksum(const uint8_t *data, uint32_t size)
{
    uint32_t hash = 2166136261u;
    uint32_t i;
    for (i = 0u; i < size; ++i) { hash ^= data[i]; hash *= 16777619u; }
    return hash;
}

static void clown_log(const KernelServices *services, uint32_t level, const char *message)
{
    if (services != NULL && services->log != NULL) services->log(services->context, level, message);
}

static void PNO_ABI_CALL clown_colour_updated(void *user_data, cc_u16f index, cc_u16f colour)
{
    uint8_t red, green, blue;
    (void)user_data;
    if (index >= 64u) return;
    red = (uint8_t)((colour >> 1) & 7u);
    green = (uint8_t)((colour >> 5) & 7u);
    blue = (uint8_t)((colour >> 9) & 7u);
    red = (uint8_t)((red << 3) | red);
    green = (uint8_t)((green << 3) | green);
    blue = (uint8_t)((blue << 3) | blue);
    clown_platform_palette_set((uint8_t)index, red, green, blue);
}

static void clown_scanline_rendered(void *user_data, cc_u16f scanline, const cc_u8l *pixels, cc_u16f left, cc_u16f right, cc_u16f width, cc_u16f height)
{
    ClownPNOState *state = (ClownPNOState *)user_data;
    Surface *surface;
    uint32_t x_offset, y_offset, dst_x, dst_y, count, max_count;
    uint8_t *dst;
    if (state == NULL || pixels == NULL) return;
    surface = clown_platform_surface();
    if (surface == NULL) return;
    if (right > width) right = width;
    if (left >= right) return;
    x_offset = surface->width > width ? (surface->width - width) / 2u : 0u;
    y_offset = surface->height > height ? (surface->height - height) / 2u : 0u;
    dst_x = x_offset + left;
    dst_y = (uint32_t)scanline + y_offset;
    if (dst_x >= surface->width || dst_y >= surface->height) return;
    count = (uint32_t)right - left;
    max_count = surface->width - dst_x;
    if (count > max_count) count = max_count;
    dst = surface->pixels + dst_y * surface->pitch + dst_x;
    memcpy(dst, pixels + left, count);
}

static cc_bool clown_input_requested(void *user_data, cc_u8f player, ClownMDEmu_Button button)
{
    (void)user_data;
    if (player != 0u) return cc_false;
    switch (button)
    {
        case CLOWNMDEMU_BUTTON_UP: return clown_platform_key_down(PNO_KEY_W) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_DOWN: return clown_platform_key_down(PNO_KEY_S) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_LEFT: return clown_platform_key_down(PNO_KEY_A) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_RIGHT: return clown_platform_key_down(PNO_KEY_D) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_A: return clown_platform_key_down(PNO_KEY_J) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_B: return clown_platform_key_down(PNO_KEY_K) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_C: return clown_platform_key_down(PNO_KEY_L) ? cc_true : cc_false;
        case CLOWNMDEMU_BUTTON_START: return clown_platform_key_down(PNO_KEY_H) ? cc_true : cc_false;
        default: return cc_false;
    }
}

// NOTE: The kernel provides the raw ROM pointer, and the PNO borrows it.
static bool clown_rom_load(ClownPNOState *state)
{
    uint8_t *raw;
    uint32_t size;

    if (state == NULL || state->services == NULL)
        return false;

    if (state->services->genesis_rom_acquire == NULL)
        return false;

    raw = NULL;
    size = 0u;

    if (state->services->genesis_rom_acquire(
        state->services->context,
        &raw,
        &size) != 0)
        return false;

    if (raw == NULL ||
        size == 0u ||
        size > CLOWN_ROM_MAX_BYTES ||
        (size & 1u) != 0u)
        return false;

    state->rom_bytes = size;
    state->rom_words = size / 2u;
    state->rom = (cc_u16l *)raw;

    return true;
}

static bool clown_sram_store(ClownPNOState *state);

static bool clown_sram_load(ClownPNOState *state)
{
    PNO_ObjectInfo info;
    uint8_t header[CLOWN_SAVE_HEADER_BYTES];
    uint8_t *sram;
    uint32_t size, checksum;
    if (state->emu->state.external_ram.size == 0u || state->emu->state.external_ram.non_volatile == 0) return true;
    size = (uint32_t)state->emu->state.external_ram.size;
    if (size > CLOWN_SRAM_MAX_BYTES || state->services->storage_find == NULL || state->services->storage_read == NULL) return false;
    memset(&info, 0, sizeof(info));
    if (state->services->storage_find(state->services->context, CLOWN_SAVE_OBJECT_NAME, &info) != 0 || info.size_bytes != CLOWN_SAVE_OBJECT_BYTES) goto erased;
    if (state->services->storage_read(state->services->context, info.object_id, 0u, sizeof(header), header) != 0) goto erased;
    if (clown_read_u32(header) != CLOWN_SAVE_MAGIC || clown_read_u32(header + 4u) != CLOWN_SAVE_VERSION || clown_read_u32(header + 8u) != size) goto erased;
    sram = (uint8_t *)state->emu->state.external_ram.buffer;
    if (state->services->storage_read(state->services->context, info.object_id, CLOWN_SAVE_HEADER_BYTES, size, sram) != 0) goto erased;
    checksum = clown_checksum(sram, size);
    if (checksum != clown_read_u32(header + 12u)) goto erased;
    clown_log(state->services, PNO_LOG_INFO, "Genesis SRAM restored");
    return true;
    erased:
    memset(state->emu->state.external_ram.buffer, 0xFF, sizeof(state->emu->state.external_ram.buffer));
    return clown_sram_store(state);
}

static bool clown_sram_store(ClownPNOState *state)
{
    PNO_ObjectInfo info;
    uint8_t header[CLOWN_SAVE_HEADER_BYTES];
    uint8_t *sram;
    uint32_t size;
    if (state == NULL || state->emu == NULL || state->emu->state.external_ram.size == 0u || state->emu->state.external_ram.non_volatile == 0) return true;
    size = (uint32_t)state->emu->state.external_ram.size;
    if (size > CLOWN_SRAM_MAX_BYTES || state->services->storage_find == NULL || state->services->storage_write == NULL || state->services->storage_sync == NULL) return false;
    memset(&info, 0, sizeof(info));
    if (state->services->storage_find(state->services->context, CLOWN_SAVE_OBJECT_NAME, &info) != 0 || info.size_bytes != CLOWN_SAVE_OBJECT_BYTES) return false;
    sram = (uint8_t *)state->emu->state.external_ram.buffer;
    clown_write_u32(header, CLOWN_SAVE_MAGIC);
    clown_write_u32(header + 4u, CLOWN_SAVE_VERSION);
    clown_write_u32(header + 8u, size);
    clown_write_u32(header + 12u, clown_checksum(sram, size));
    if (state->services->storage_write(state->services->context, info.object_id, CLOWN_SAVE_HEADER_BYTES, size, sram) != 0) return false;
    if (state->services->storage_write(state->services->context, info.object_id, 0u, sizeof(header), header) != 0) return false;
    if (state->services->storage_sync(state->services->context) != 0) return false;
    return true;
}

static void clown_pno_cleanup(ClownPNOState *state)
{
    const KernelServices *services;
    if (state == NULL) return;
    services = state->services;
    if (state->audio_started)
    {
        clown_audio_deactivate();
        state->audio_started = false;
    }
    if (state->initialized) (void)clown_sram_store(state);
    clown_platform_shutdown();
    if (services != NULL && services->mem_free != NULL)
    {
        if (state->emu != NULL) services->mem_free(services->context, state->emu);
        services->mem_free(services->context, state);
    }
}

static void clown_rom_prepare(ClownPNOState *state)
{
    uint8_t *raw;
    uint32_t i;

    raw = (uint8_t *)state->rom;

    for (i = 0u; i < state->rom_words; ++i)
    {
        state->rom[i] = (cc_u16l)(
            ((uint16_t)raw[i * 2u] << 8) |
            (uint16_t)raw[i * 2u + 1u]);
    }
}

static int32_t PNO_ABI_CALL clown_pno_init(const KernelServices *services, void **program_state)
{
    ClownPNOState *state;
    ClownMDEmu_InitialConfiguration config;
    if (program_state == NULL || services == NULL) return -1;
    *program_state = NULL;
    if (PNO_ABI_ValidateServices(services) != PNO_ABI_OK) return -2;
    if ((services->capabilities & (PNO_CAP_MEMORY | PNO_CAP_STORAGE_READ | PNO_CAP_VIDEO | PNO_CAP_VIDEO_PALETTE)) != (PNO_CAP_MEMORY | PNO_CAP_STORAGE_READ | PNO_CAP_VIDEO | PNO_CAP_VIDEO_PALETTE)) return -3;
    if (services->mem_alloc == NULL || services->mem_free == NULL) return -4;
    state = (ClownPNOState *)services->mem_alloc(services->context, (uint32_t)sizeof(*state), 16u);
    if (state == NULL) return -5;
    memset(state, 0, sizeof(*state));
    state->services = services;
    if (!clown_platform_init(services)) { clown_pno_cleanup(state); return -6; }
    state->emu = (ClownMDEmu *)services->mem_alloc(services->context, (uint32_t)sizeof(*state->emu), 16u);
    if (state->emu == NULL) { clown_pno_cleanup(state); return -7; }
    memset(state->emu, 0, sizeof(*state->emu));
    if (!clown_rom_load(state)) { clown_log(services, PNO_LOG_ERROR, "GenesisROM FAS object missing or invalid"); clown_pno_cleanup(state); return -8; }
    clown_rom_prepare(state);
    ClownMDEmu_Constant_Initialise();
    memset(&config, 0, sizeof(config));
    config.general.region = CLOWNMDEMU_REGION_OVERSEAS;
    config.general.tv_standard = CLOWNMDEMU_TV_STANDARD_NTSC;
    config.general.low_pass_filter_disabled = cc_false;
    config.general.cd_add_on_enabled = cc_false;
    memset(&state->callbacks, 0, sizeof(state->callbacks));
    state->callbacks.user_data = state;
    state->callbacks.colour_updated = clown_colour_updated;
    state->callbacks.scanline_rendered = clown_scanline_rendered;
    state->callbacks.input_requested = clown_input_requested;
    state->callbacks.fm_audio_to_be_generated = clown_audio_fm;
    state->callbacks.psg_audio_to_be_generated = clown_audio_psg;
    state->callbacks.pcm_audio_to_be_generated = clown_audio_pcm;
    state->callbacks.cdda_audio_to_be_generated = clown_audio_cdda;
    ClownMDEmu_Initialise(state->emu, &config, &state->callbacks);
    ClownMDEmu_SetCartridge(state->emu, state->rom, state->rom_words);
    ClownMDEmu_HardReset(state->emu, cc_true, cc_false);
    (void)clown_sram_load(state);
    clown_platform_clear(0u);
    state->initialized = true;
    *program_state = state;
    clown_log(services, PNO_LOG_INFO, "clownMDEmu PNO initialized");
    return 0;
}

static void PNO_ABI_CALL clown_pno_update(void *program_state, uint32_t delta_ticks)
{
    ClownPNOState *state = (ClownPNOState *)program_state;
    if (state == NULL || !state->initialized) return;
    clown_platform_begin_frame(delta_ticks);
    ClownMDEmu_Iterate(state->emu);
    clown_audio_mix_pending();
    clown_platform_end_frame();
}

static void PNO_ABI_CALL clown_pno_handle_event(void *program_state, const PNO_Event *event)
{
    ClownPNOState *state = (ClownPNOState *)program_state;
    if (state == NULL || !state->initialized || event == NULL) return;
    clown_platform_handle_event(event);
}

static void PNO_ABI_CALL clown_pno_on_activate(void *program_state)
{
    ClownPNOState *state = (ClownPNOState *)program_state;
    if (state == NULL || !state->initialized || state->audio_started) return;
    state->audio_started = clown_audio_activate();
    if (!state->audio_started)
        clown_log(state->services, PNO_LOG_WARNING, "CLOWN audio activation failed");
}

static void PNO_ABI_CALL clown_pno_on_deactivate(void *program_state)
{
    ClownPNOState *state = (ClownPNOState *)program_state;
    if (state == NULL || !state->initialized) return;
    if (state->audio_started)
    {
        clown_audio_deactivate();
        state->audio_started = false;
    }
    if (!clown_sram_store(state))
        clown_log(state->services, PNO_LOG_WARNING, "Genesis SRAM save failed");
}

static void PNO_ABI_CALL clown_pno_shutdown(void *program_state)
{
    clown_pno_cleanup((ClownPNOState *)program_state);
}

int32_t PNO_ABI_CALL pno_entry(const KernelServices *services, FAS_ProgramAPI *program_api)
{
    if (services == NULL || program_api == NULL) return -1;
    memset(program_api, 0, sizeof(*program_api));
    program_api->struct_size = sizeof(*program_api);
    program_api->abi_major = PNO_ABI_VERSION_MAJOR;
    program_api->abi_minor = PNO_ABI_VERSION_MINOR;
    memcpy(program_api->name, "CLOWN", 6u);
    program_api->init = clown_pno_init;
    program_api->update = clown_pno_update;
    program_api->handle_event = clown_pno_handle_event;
    program_api->on_activate = clown_pno_on_activate;
    program_api->on_deactivate = clown_pno_on_deactivate;
    program_api->shutdown = clown_pno_shutdown;
    return 0;
}
