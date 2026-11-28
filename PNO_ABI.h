
// PNO_ABI.h    NOTE: "Penetrizzel Native Object"

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#ifndef PNO_ABI_H
#define PNO_ABI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PNO_ABI_VERSION_MAJOR       1u
#define PNO_ABI_VERSION_MINOR       5u      // WARNING + NOTE: "outdated" Games wont run and Return a init Error -2!
#if defined(__i386__)
#define PNO_ABI_CALL                __attribute__((cdecl))
#else
#define PNO_ABI_CALL
#endif

#define PNO_PROGRAM_NAME_MAX        31u
#define PNO_EVENT_TEXT_MAX          16u

#define PNO_CAP_MEMORY              0x00000001u
#define PNO_CAP_LOG                 0x00000002u
#define PNO_CAP_TIME                0x00000004u
#define PNO_CAP_STORAGE_READ        0x00000008u
#define PNO_CAP_STORAGE_WRITE       0x00000010u
#define PNO_CAP_VIDEO               0x00000020u
#define PNO_CAP_AUDIO               0x00000040u
#define PNO_CAP_VIDEO_PALETTE       0x00000100u
#define PNO_SERVICE_VIDEO_PALETTE   8u

#define PNO_EVENT_NONE              0u
#define PNO_EVENT_KEY_DOWN          1u
#define PNO_EVENT_KEY_UP            2u
#define PNO_EVENT_TEXT              3u
#define PNO_EVENT_POINTER_MOVE      4u
#define PNO_EVENT_POINTER_BUTTON    5u
#define PNO_EVENT_QUIT              6u

#define PNO_PIXEL_INDEX8             1u

#define PNO_LOG_DEBUG               0u
#define PNO_LOG_INFO                1u
#define PNO_LOG_WARNING             2u
#define PNO_LOG_ERROR               3u

#define PNO_SERVICE_MEMORY          1u
#define PNO_SERVICE_LOG             2u
#define PNO_SERVICE_TIME            3u
#define PNO_SERVICE_STORAGE         4u
#define PNO_SERVICE_VIDEO           5u
#define PNO_SERVICE_AUDIO           6u

#define PNO_SERVICE_VERSION_1       1u

#define PNO_CAP_STORAGE_SYNC        0x00000080u
#define PNO_SERVICE_STORAGE_SYNC    7u
#define PNO_CAP_GAME_SAVE           0x00000200u
#define PNO_SERVICE_GAME_SAVE       9u

// Chess
enum {
    PNO_AUDIO_ASSET_MUSIC = 1u << 0,
    PNO_AUDIO_ASSET_LOOP  = 1u << 1
};

// PNO-owned enumeration; preserves the numeric ordering of your existing kernel KeyCode enumeration
typedef enum
{
    PNO_KEY_NONE = 0,
    PNO_KEY_A, PNO_KEY_B, PNO_KEY_C, PNO_KEY_D,
    PNO_KEY_E, PNO_KEY_F, PNO_KEY_G, PNO_KEY_H,
    PNO_KEY_I, PNO_KEY_J, PNO_KEY_K, PNO_KEY_L,
    PNO_KEY_M, PNO_KEY_N, PNO_KEY_O, PNO_KEY_P,
    PNO_KEY_Q, PNO_KEY_R, PNO_KEY_S, PNO_KEY_T,
    PNO_KEY_U, PNO_KEY_V, PNO_KEY_W, PNO_KEY_X,
    PNO_KEY_Y, PNO_KEY_Z,
    PNO_KEY_0, PNO_KEY_1, PNO_KEY_2, PNO_KEY_3,
    PNO_KEY_4, PNO_KEY_5, PNO_KEY_6, PNO_KEY_7,
    PNO_KEY_8, PNO_KEY_9,
    PNO_KEY_SPACE,
    PNO_KEY_MINUS, PNO_KEY_EQUAL,
    PNO_KEY_LEFT_BRACKET, PNO_KEY_RIGHT_BRACKET,
    PNO_KEY_SEMICOLON, PNO_KEY_APOSTROPHE,
    PNO_KEY_COMMA, PNO_KEY_PERIOD, PNO_KEY_SLASH,
    PNO_KEY_BACKSLASH,
    PNO_KEY_ENTER, PNO_KEY_BACKSPACE, PNO_KEY_TAB,
    PNO_KEY_ESCAPE,
    PNO_KEY_F1, PNO_KEY_F2, PNO_KEY_F3,
    PNO_KEY_F4, PNO_KEY_F5, PNO_KEY_F6,
    PNO_KEY_F7, PNO_KEY_F8, PNO_KEY_F9,
    PNO_KEY_SHIFT, PNO_KEY_CTRL, PNO_KEY_ALT,
    PNO_KEY_UP, PNO_KEY_DOWN, PNO_KEY_LEFT,
    PNO_KEY_RIGHT,
    PNO_KEY_HOME, PNO_KEY_END, PNO_KEY_DELETE,
    PNO_KEY_COUNT

} PNO_KeyCode;

typedef enum
{
    PNO_ABI_OK = 0,
    PNO_ABI_ERR_ARGUMENT,
    PNO_ABI_ERR_SIZE,
    PNO_ABI_ERR_VERSION,
    PNO_ABI_ERR_CAPABILITY,
    PNO_ABI_ERR_CALLBACKS

} PNO_ABI_Status;

typedef struct
{
    uint32_t type;
    uint32_t code;
    int32_t value;
    int32_t x;
    int32_t y;
    uint32_t modifiers;
    uint32_t ticks;

} PNO_Event;

typedef struct
{
    uint32_t object_id;
    uint64_t size_bytes;
    uint64_t generation;
    uint32_t flags;
    char name[32];

} PNO_ObjectInfo;

typedef struct
{
    uint32_t struct_size;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t pixel_format;
    uint8_t *pixels;

} PNO_Surface;

typedef void *(PNO_ABI_CALL *PNO_MemAllocFn)(void *context, uint32_t size, uint32_t alignment);
typedef void (PNO_ABI_CALL *PNO_MemFreeFn)(void *context, void *ptr);
typedef void (PNO_ABI_CALL *PNO_LogFn)(void *context, uint32_t level, const char *message);
typedef uint32_t (PNO_ABI_CALL *PNO_GetTicksFn)(void *context);
typedef int32_t (PNO_ABI_CALL *PNO_StorageFindFn)(void *context, const char *name, PNO_ObjectInfo *info);
typedef int32_t (PNO_ABI_CALL *PNO_StorageReadFn)(void *context, uint32_t object_id, uint64_t offset, uint32_t length, void *buffer);
typedef int32_t (PNO_ABI_CALL *PNO_StorageWriteFn)(void *context, uint32_t object_id, uint64_t offset, uint32_t length, const void *buffer);
typedef int32_t (PNO_ABI_CALL *PNO_VideoGetSurfaceFn)(void *context, PNO_Surface *surface);
typedef void (PNO_ABI_CALL *PNO_VideoClearFn)(void *context, uint8_t color);
typedef int32_t (PNO_ABI_CALL *PNO_AudioSetEnabledFn)(void *context, bool enabled);
typedef int32_t (PNO_ABI_CALL *PNO_StorageSyncFn)(void *context);
typedef void (PNO_ABI_CALL *PNO_VideoSetPaletteFn)(void *context, uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
typedef void (PNO_ABI_CALL *PNO_AudioPCMFillFn)(void *user_data, int16_t *samples, uint32_t frames);
typedef int32_t (PNO_ABI_CALL *PNO_AudioPCMStartFn)(void *context, PNO_AudioPCMFillFn callback, void *user_data);
typedef int32_t (PNO_ABI_CALL *PNO_AudioPCMStopFn)(void *context);
typedef int32_t (PNO_ABI_CALL *PNO_AudioTonePlayFn)(void *context, uint32_t frequency, uint32_t duration_ticks);
typedef int32_t (PNO_ABI_CALL *PNO_AudioToneSetFrequencyFn)(void *context, uint32_t frequency);
typedef int32_t (PNO_ABI_CALL *PNO_AudioToneStopFn)(void *context);
typedef int32_t (PNO_ABI_CALL *PNO_AudioGetEnabledFn)(void *context);

typedef bool (PNO_ABI_CALL *PNO_SaveExportFn)(void *context, uint8_t *payload, uint32_t capacity, uint32_t *payload_size);
typedef bool (PNO_ABI_CALL *PNO_SaveImportFn)(void *context, const uint8_t *payload, uint32_t payload_size);
typedef int32_t (PNO_ABI_CALL *PNO_GameSaveRegisterFn)(void *context, const char *object_name, uint32_t max_payload_size, PNO_SaveExportFn export_data, PNO_SaveImportFn import_data, void *provider_context);
typedef int32_t (PNO_ABI_CALL *PNO_GameSaveStoreFn)(void *context);
typedef int32_t (PNO_ABI_CALL *PNO_GameSaveUnregisterFn)(void *context);

typedef int32_t (PNO_ABI_CALL *PNO_GenesisROMAcquireFn)(void *context, uint8_t **rom, uint32_t *size);

typedef int32_t (PNO_ABI_CALL *PNO_AudioBlobAcquireFn)(void *context, const uint8_t **blob, uint32_t *size);
typedef int32_t (PNO_ABI_CALL *PNO_AudioAssetPlayFn)(void *context, const char *name, uint32_t flags);
typedef int32_t (PNO_ABI_CALL *PNO_AudioAssetStopFn)(void *context, const char *name, uint32_t flags);

typedef struct KernelServices
{
    uint32_t struct_size;
    uint16_t abi_major;
    uint16_t abi_minor;
    uint32_t capabilities;
    void *context;

    PNO_MemAllocFn mem_alloc;
    PNO_MemFreeFn mem_free;
    PNO_LogFn log;
    PNO_GetTicksFn get_ticks;

    PNO_StorageFindFn storage_find;
    PNO_StorageReadFn storage_read;
    PNO_StorageWriteFn storage_write;

    PNO_VideoGetSurfaceFn video_get_surface;
    PNO_VideoClearFn video_clear;

    PNO_AudioSetEnabledFn audio_set_enabled;
    PNO_StorageSyncFn storage_sync;
    PNO_VideoSetPaletteFn video_set_palette;

    PNO_AudioGetEnabledFn audio_get_enabled;
    PNO_AudioPCMStartFn audio_pcm_start;
    PNO_AudioPCMStopFn audio_pcm_stop;
    PNO_AudioTonePlayFn audio_tone_play;
    PNO_AudioToneSetFrequencyFn audio_tone_set_frequency;
    PNO_AudioToneStopFn audio_tone_stop;

    PNO_GameSaveRegisterFn game_save_register;
    PNO_GameSaveStoreFn game_save_store;
    PNO_GameSaveUnregisterFn game_save_unregister;

    PNO_GenesisROMAcquireFn genesis_rom_acquire;

    PNO_AudioBlobAcquireFn audio_blob_acquire;
    PNO_AudioAssetPlayFn audio_asset_play;
    PNO_AudioAssetStopFn audio_asset_stop;

} KernelServices;

typedef int32_t (PNO_ABI_CALL *PNO_ProgramInitFn)(const KernelServices *services, void **state);
typedef void (PNO_ABI_CALL *PNO_ProgramUpdateFn)(void *state, uint32_t delta_ticks);
typedef void (PNO_ABI_CALL *PNO_ProgramEventFn)(void *state, const PNO_Event *event);
typedef void (PNO_ABI_CALL *PNO_ProgramLifecycleFn)(void *state);

typedef struct
{
    uint32_t struct_size;
    uint16_t abi_major;
    uint16_t abi_minor;
    uint32_t flags;
    char name[PNO_PROGRAM_NAME_MAX + 1u];

    PNO_ProgramInitFn init;
    PNO_ProgramUpdateFn update;
    PNO_ProgramEventFn handle_event;
    PNO_ProgramLifecycleFn on_activate;
    PNO_ProgramLifecycleFn on_deactivate;
    PNO_ProgramLifecycleFn shutdown;
} FAS_ProgramAPI;

typedef int32_t (PNO_ABI_CALL *PNO_EntryFn)(const KernelServices *services, FAS_ProgramAPI *program_api);

PNO_ABI_Status PNO_ABI_ValidateServices(const KernelServices *services);
PNO_ABI_Status PNO_ABI_ValidateProgramAPI(const FAS_ProgramAPI *program_api);
const char *PNO_ABI_StatusString(PNO_ABI_Status status);

#endif

