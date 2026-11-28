
// PNO_ABI.c

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#include "PNO_ABI.h"

#include <stddef.h>

#define PNO_FIELD_END(type, field) ((uint32_t)(offsetof(type, field) + sizeof(((type *)0)->field)))

PNO_ABI_Status PNO_ABI_ValidateServices(const KernelServices *services)
{
    if (services == NULL)
        return PNO_ABI_ERR_ARGUMENT;

    if (services->struct_size < PNO_FIELD_END(KernelServices, get_ticks))
        return PNO_ABI_ERR_SIZE;

    if (services->abi_major != PNO_ABI_VERSION_MAJOR || services->abi_minor > PNO_ABI_VERSION_MINOR)
        return PNO_ABI_ERR_VERSION;

    if ((services->capabilities & PNO_CAP_MEMORY) != 0u && (services->mem_alloc == NULL || services->mem_free == NULL))
        return PNO_ABI_ERR_CALLBACKS;

    if ((services->capabilities & PNO_CAP_LOG) != 0u && services->log == NULL)
        return PNO_ABI_ERR_CALLBACKS;

    if ((services->capabilities & PNO_CAP_TIME) != 0u && services->get_ticks == NULL)
        return PNO_ABI_ERR_CALLBACKS;

    if ((services->capabilities & PNO_CAP_STORAGE_READ) != 0u && (services->struct_size < PNO_FIELD_END(KernelServices, storage_read) || services->storage_find == NULL || services->storage_read == NULL))
        return PNO_ABI_ERR_CALLBACKS;

    if ((services->capabilities & PNO_CAP_STORAGE_WRITE) != 0u && (services->struct_size < PNO_FIELD_END(KernelServices, storage_write) || services->storage_write == NULL))
        return PNO_ABI_ERR_CALLBACKS;

    if ((services->capabilities & PNO_CAP_VIDEO) != 0u && (services->struct_size < PNO_FIELD_END(KernelServices, video_clear) || services->video_get_surface == NULL || services->video_clear == NULL))
        return PNO_ABI_ERR_CALLBACKS;

    if ((services->capabilities & PNO_CAP_AUDIO) != 0u && (services->struct_size < PNO_FIELD_END(KernelServices, audio_set_enabled) || services->audio_set_enabled == NULL))
        return PNO_ABI_ERR_CALLBACKS;

    if ((services->capabilities & PNO_CAP_STORAGE_SYNC) != 0u && (services->struct_size < PNO_FIELD_END(KernelServices, storage_sync) || services->storage_sync == NULL))
        return PNO_ABI_ERR_CALLBACKS;

    if ((services->capabilities & PNO_CAP_VIDEO_PALETTE) != 0u && (services->struct_size < PNO_FIELD_END(KernelServices, video_set_palette) || services->video_set_palette == NULL))
        return PNO_ABI_ERR_CALLBACKS;

    if ((services->capabilities & PNO_CAP_GAME_SAVE) != 0u && (services->struct_size < PNO_FIELD_END(KernelServices, game_save_unregister)
        || services->game_save_register == NULL || services->game_save_store == NULL || services->game_save_unregister == NULL))
        return PNO_ABI_ERR_CALLBACKS;

    return PNO_ABI_OK;
}

PNO_ABI_Status PNO_ABI_ValidateProgramAPI(const FAS_ProgramAPI *program_api)
{
    uint32_t i;
    bool has_name = false;

    if (program_api == NULL)
        return PNO_ABI_ERR_ARGUMENT;

    if (program_api->struct_size < PNO_FIELD_END(FAS_ProgramAPI, shutdown))
        return PNO_ABI_ERR_SIZE;

    if (program_api->abi_major != PNO_ABI_VERSION_MAJOR || program_api->abi_minor > PNO_ABI_VERSION_MINOR)
        return PNO_ABI_ERR_VERSION;

    if (program_api->init == NULL || program_api->update == NULL || program_api->shutdown == NULL)
        return PNO_ABI_ERR_CALLBACKS;

    for (i = 0u; i <= PNO_PROGRAM_NAME_MAX; ++i)
    {
        if (program_api->name[i] == '\0')
        {
            has_name = (i != 0u);
            break;
        }
    }

    if (!has_name)
        return PNO_ABI_ERR_ARGUMENT;

    return PNO_ABI_OK;
}

const char *PNO_ABI_StatusString(PNO_ABI_Status status)
{
    static const char *const names[] =
    {
        "OK",
        "invalid argument",
        "structure too small",
        "ABI version mismatch",
        "missing capability",
        "invalid callback table"
    };

    if ((uint32_t)status >= (uint32_t)(sizeof(names) / sizeof(names[0])))
        return "unknown ABI status";

    return names[(uint32_t)status];
}
