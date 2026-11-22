
// clown_input.c

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#include "clown_input.h"
#include "../../keyboard.h"

#include <string.h>

static MDController g_controller[2];

void clown_input_poll(void)
{
    // Player 1
    // D-PAD    🎮
    g_controller[0].up      = keyboard_is_down(KEY_W); // UP
    g_controller[0].down    = keyboard_is_down(KEY_S); // DOWN
    g_controller[0].left    = keyboard_is_down(KEY_A); // LEFT
    g_controller[0].right   = keyboard_is_down(KEY_D); // RIGHT

    // Buttons  🕹️
    g_controller[0].a       = keyboard_is_down(KEY_J); // A
    g_controller[0].b       = keyboard_is_down(KEY_K); // B
    g_controller[0].c       = keyboard_is_down(KEY_L); // C
    g_controller[0].start   = keyboard_is_down(KEY_H); // START

    // Player 2 is currently unused.    TODO
    memset(&g_controller[1], 0, sizeof(g_controller[1]));
}

const MDController* clown_input_get_controller(unsigned player)
{
    return &g_controller[player];
}
