
// clown_input.h

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#ifndef CLOWN_INPUT_H
#define CLOWN_INPUT_H


#include <stdint.h>

typedef struct
{
    uint8_t up;
    uint8_t down;
    uint8_t left;
    uint8_t right;

    uint8_t a;
    uint8_t b;
    uint8_t c;

    uint8_t start;

} MDController;

void clown_input_poll(void);

const MDController* clown_input_get_controller(unsigned player);

#endif
