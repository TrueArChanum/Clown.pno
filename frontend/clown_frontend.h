
// clown_frontend.h

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#ifndef CLOWN_FRONTEND_H
#define CLOWN_FRONTEND_H

#include "../../surface.h"
#include <stdbool.h>

bool clown_init(Surface *surface);
void clown_shutdown(void);
void clown_frame(void);

//BEGIN SRAM
#include "../../game_save.h"
#define CLOWN_SRAM_MAX_BYTES GAME_SAVE_GENESIS_MAX_BYTES    // 64 KiB; game_save.h

bool clown_sram_get_info(size_t *size, bool *non_volatile);
bool clown_sram_copy_in(const void *buffer, size_t size);
bool clown_sram_copy_out(void *buffer, size_t capacity, size_t *bytes_read);

// API
bool kernel_genesis_sram_attach(void);
bool kernel_genesis_sram_store_before_power_transition(void);
bool kernel_genesis_sram_checkpoint(void);
//END SRAM

#endif
