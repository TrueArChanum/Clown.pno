
// clown_frontend.c

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#include "clown_frontend.h"
#include "../../include/RAM_map.h"
#include "clown_input.h"
#include "clown_audio.h"
#include "clown_video.h"

// for memcpy(); SRAM stuff
#include "../../libc_compat.h" // IWYU pragma: keep
#include "../../pool.h"

#include <stdint.h>
#include <stddef.h>

#include "../../ROM/ROM.h"
#include "../../ROM/genesis_rom.h" // IWYU pragma: keep

#define CLOWN_ROM_MAX_BYTES RAM_ROM_MAX_SIZE
#define CLOWN_ROM_MAX_WORDS (CLOWN_ROM_MAX_BYTES / 2u)


// NOTE: allocating the ROM twice in RAM for the ROM RESET.
// NOTE: ROM is stored as big-endian bytes; cc_u16l is little-endian on x86.
const cc_u16l *ROM_BASE_PTR = NULL;

typedef struct ClownMachine
{
    ClownMDEmu clown;
    cc_u16l *rom;
    size_t rom_words;
    Surface *surface;
    ClownMDEmu_Callbacks callbacks;

} ClownMachine;

static ClownMachine *machine = NULL;


//BEGIN SRAM

static void genesis_sram_initialize_erased(void)
{
    memset(machine->clown.state.external_ram.buffer, 0xFF, sizeof(machine->clown.state.external_ram.buffer));   // set SRAM to "FF" initial
}

bool clown_sram_get_info(size_t *size, bool *non_volatile)
{
    if (machine == NULL || size == NULL || non_volatile == NULL)        return false;

    *size = 0u;
    *non_volatile = false;

    if (machine->clown.state.external_ram.size == 0u)                   return false;
    if (machine->clown.state.external_ram.size > CLOWN_SRAM_MAX_BYTES)  return false;

    *size = (size_t)machine->clown.state.external_ram.size;
    *non_volatile = machine->clown.state.external_ram.non_volatile != 0;

    return true;
}

bool clown_sram_copy_in(const void *buffer, size_t size)
{
    size_t sram_size;
    bool non_volatile;

    if (buffer == NULL)                                     return false;
    if (!clown_sram_get_info(&sram_size, &non_volatile))    return false;
    if (!non_volatile || size != sram_size)                 return false;

    memcpy(machine->clown.state.external_ram.buffer, buffer, sram_size);

    return true;
}

bool clown_sram_copy_out(void *buffer, size_t capacity, size_t *bytes_read)
{
    size_t sram_size;
    bool non_volatile;

    if (bytes_read != NULL) *bytes_read = 0u;

    if (buffer == NULL || bytes_read == NULL)               return false;
    if (!clown_sram_get_info(&sram_size, &non_volatile))    return false;
    if (!non_volatile || capacity < sram_size)              return false;

    // Live emulator SRAM 💾
    memcpy(buffer, machine->clown.state.external_ram.buffer, sram_size);

    *bytes_read = sram_size;

    return true;
}

// Kernel-side attachment to the persistent buffer
static uint8_t g_genesis_sram[GAME_SAVE_GENESIS_MAX_BYTES]; // Kernel-side snapshot
static size_t g_genesis_sram_size;
static bool g_genesis_sram_initialized;

bool kernel_genesis_sram_attach(void)
{
    size_t detected_size;
    size_t loaded_size = 0u;
    bool non_volatile;
    FAS_Status status;

    if (!clown_sram_get_info(&detected_size, &non_volatile))                                return false;
    if (!non_volatile || detected_size == 0u || detected_size > sizeof(g_genesis_sram))     return false;
    if (g_genesis_sram_initialized && g_genesis_sram_size != detected_size)                 return false;

    if (!g_genesis_sram_initialized)
    {
        status = game_save_genesis_load(g_genesis_sram, sizeof(g_genesis_sram), &loaded_size);

        if (status == FAS_OK)
        {
            if (loaded_size != detected_size)   return false;
        }

        else if (status == FAS_ERR_NOT_FOUND)
        {
            genesis_sram_initialize_erased();

            if (!clown_sram_copy_out(g_genesis_sram, sizeof(g_genesis_sram), &loaded_size) || loaded_size != detected_size) return false;

            status = game_save_genesis_store(g_genesis_sram, detected_size);
            if (status != FAS_OK)
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        g_genesis_sram_size = detected_size;
        g_genesis_sram_initialized = true;
    }

    if (!clown_sram_copy_in(g_genesis_sram, g_genesis_sram_size))   return false;

    return true;
}

static bool kernel_genesis_sram_capture(void)
{
    size_t captured_size;

    if (!g_genesis_sram_initialized)                                                    return false;
    if (!clown_sram_copy_out(g_genesis_sram, sizeof(g_genesis_sram), &captured_size))   return false;

    return captured_size == g_genesis_sram_size;
}

// Save before reboot or shutdown
bool kernel_genesis_sram_store_before_power_transition(void)
{
    FAS_Status status;

    if (!g_genesis_sram_initialized)    return false;
    if (!kernel_genesis_sram_capture()) return false;

    status = game_save_genesis_store(g_genesis_sram, g_genesis_sram_size);

    if (status != FAS_OK)               return false;

    return true;
}

// F2 Soft Reset
bool kernel_genesis_sram_checkpoint(void)
{
    return kernel_genesis_sram_store_before_power_transition();
}

//END SRAM


static void clown_colour_updated(void *user_data, cc_u16f index, cc_u16f colour)
{
    (void)user_data; // not used...

    clown_video_colour_update(index, colour);
}

static void clown_scanline_rendered(
    void *user_data,
    cc_u16f scanline,
    const cc_u8l *pixels,
    cc_u16f left,
    cc_u16f right,
    cc_u16f width,
    cc_u16f height)
{
    ClownMachine *clown_machine = (ClownMachine *)user_data;

    clown_video_scanline(
        clown_machine->surface,
        scanline,
        pixels,
        left,
        right,
        width,
        height
    );
}

static cc_bool clown_input_requested(void *user_data, cc_u8f player, ClownMDEmu_Button button)
{
    (void)user_data; // not used...

    if (player >= 2)    return cc_false;

    const MDController *pad = clown_input_get_controller(player);

    if (pad == NULL)    return cc_false;

    switch(button)
    {
        case CLOWNMDEMU_BUTTON_UP:
            return pad->up;

        case CLOWNMDEMU_BUTTON_DOWN:
            return pad->down;

        case CLOWNMDEMU_BUTTON_LEFT:
            return pad->left;

        case CLOWNMDEMU_BUTTON_RIGHT:
            return pad->right;

        case CLOWNMDEMU_BUTTON_A:
            return pad->a;

        case CLOWNMDEMU_BUTTON_B:
            return pad->b;

        case CLOWNMDEMU_BUTTON_C:
            return pad->c;

        case CLOWNMDEMU_BUTTON_START:
            return pad->start;

        default:
            return cc_false;
    }
}

// Convert raw ROM bytes into 68000 big-endian words.
static void prepare_rom(void)
{
    const uint8_t *src = (const uint8_t *)(uintptr_t)RAM_ROM_BASE;

    cc_u16l *dst = (cc_u16l *)(uintptr_t)RAM_ROM_BASE;

    for (size_t i = 0; i < machine->rom_words; ++i)
    {
        dst[i] = ((cc_u16l)src[i * 2] << 8) | (cc_u16l)src[i * 2 + 1];
    }
}

//DE-BUG
// #include "kprintf.h"

static bool clown_frontend_restart(void)
{
    if (machine == NULL)
        return false;

    ClownMDEmu_HardReset(
        &machine->clown,
        cc_true,
        cc_false
    );

    return true;
}

bool clown_init(Surface *surface)
{
    //DE-BUG
    // PoolStats before, after;
    // pool_get_stats(&before);
    //DE-BUG

    if (surface == NULL || machine != NULL) return false;

    machine = pool_alloc(sizeof(*machine));

    if (machine == NULL) return false;

    memset(machine, 0, sizeof(*machine));

    g_genesis_sram_size = 0u;
    g_genesis_sram_initialized = false;

    machine->surface = surface;
    machine->rom_words = clown_rom_get_size_words();    // genesis_rom.c
    machine->rom = (cc_u16l *)(uintptr_t)RAM_ROM_BASE;

    if (machine->rom_words == 0u || machine->rom_words > CLOWN_ROM_MAX_WORDS)
    {
        clown_shutdown();
        return false;
    }

    memcpy((void *)(uintptr_t)RAM_ROM_BASE, (const void *)(uintptr_t)RAM_ROM_BACKUP_BASE, machine->rom_words * sizeof(uint16_t));   // HACK to fix F2 Soft Reset.

    prepare_rom();

    machine->callbacks = (ClownMDEmu_Callbacks)
    {
        .user_data = machine,
        .colour_updated = clown_colour_updated,
        .scanline_rendered = clown_scanline_rendered,
        .input_requested = clown_input_requested,
        .fm_audio_to_be_generated = clown_audio_fm,
        .psg_audio_to_be_generated = clown_audio_psg,
        .pcm_audio_to_be_generated = clown_audio_pcm,
        .cdda_audio_to_be_generated = clown_audio_cdda
    };

    ClownMDEmu_Constant_Initialise();

    ClownMDEmu_InitialConfiguration config = {0};

    config.general.region = CLOWNMDEMU_REGION_OVERSEAS;
    config.general.tv_standard = CLOWNMDEMU_TV_STANDARD_NTSC;
    config.general.low_pass_filter_disabled = cc_false;
    config.general.cd_add_on_enabled = cc_false;

    ClownMDEmu_Initialise(
        &machine->clown,
        &config,
        &machine->callbacks);

    clown_video_init(surface);

    ClownMDEmu_SetCartridge(
        &machine->clown,
        machine->rom,
        (cc_u32f)machine->rom_words);

    ClownMDEmu_HardReset(&machine->clown, cc_true, cc_false);

    clown_audio_reset();    // 🎶

    //DE-BUG
    // pool_get_stats(&after);
    // kprintf("CLOWN memory: %u bytes\n", (uint32_t)(after.allocated_bytes - before.allocated_bytes));
    //NOTE: CLOWN memory: 1132224 bytes
    //DE-BUG

    return true;
}

void clown_shutdown(void)
{
    ClownMachine *old_machine = machine;

    if (old_machine == NULL)
        return;

    clown_audio_deactivate();

    machine = NULL;

    pool_free(old_machine);
}

void clown_frame(void)
{
    if (machine == NULL)    return;

    clown_input_poll();                  // 🎮 + ⌨️

    ClownMDEmu_Iterate(&machine->clown); // 📠

    clown_audio_mix_pending();  // 🎶
}


/* NOTE:
 *
 * ClownMDEmu_Iterate
 * │
 * ├── calculate frame/scanline timing
 * │
 * ├── for EVERY scanline
 * │   │
 * │   ├── VDP_BeginScanline()
 * │   │
 * │   ├── SyncM68k()
 * │   │
 * │   ├── render scanline
 * │   │   └── VDP_EndScanline()
 * │   │
 * │   ├── SyncM68k()
 * │   │
 * │   ├── H-INT handling
 * │   │
 * │   └── scanline timing
 * │
 * ├── SyncM68k()
 * ├── SyncZ80()
 * ├── SyncMCDM68k()
 * ├── SyncFM()
 * ├── SyncPSG()
 * ├── SyncPCM()
 * ├── SyncCDDA()
 * ├── SyncIOPort() × N
 * ├── IRQ handling
 * └── CDDA fade
 *
 */
