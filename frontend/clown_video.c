
// clown_video.c

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#include "clown_video.h"
#include "../../vga_palette.h"
#include "../../SIMD.h"

#include <stdint.h>

#define MD_CRAM_ENTRIES 64

static Surface *surface = NULL;

static uint16_t cram[MD_CRAM_ENTRIES];

void clown_video_init(Surface *s)
{
    surface = s;

    for (uint32_t i = 0; i < MD_CRAM_ENTRIES; ++i)
    {
        cram[i] = 0;

        // removed palette clearing, cause...
        // vga_palette_set( // BUG wipes wurm vga_palette too!
        //     (uint8_t)i,
        //                 0,
        //                 0,
        //                 0
        // );
    }
}

void clown_video_colour_update(
    cc_u16f index,
    cc_u16f colour)
{
    if (index >= MD_CRAM_ENTRIES)   return;

    cram[index] = colour;

    uint8_t r = (uint8_t)((colour >> 1) & 0x7);
    uint8_t g = (uint8_t)((colour >> 5) & 0x7);
    uint8_t b = (uint8_t)((colour >> 9) & 0x7);

    // Mega Drive: 3 bits/channel
    // VGA DAC:    6 bits/channel
    r = (uint8_t)((r << 3) | r);
    g = (uint8_t)((g << 3) | g);
    b = (uint8_t)((b << 3) | b);

    vga_palette_set(
        (uint8_t)index,
                    r,
                    g,
                    b
    );
}

void clown_video_scanline(
    Surface *dst_surface,
    cc_u16f scanline,
    const cc_u8l *pixels,
    cc_u16f left,
    cc_u16f right,
    cc_u16f width,
    cc_u16f height)
{
    if (dst_surface == NULL || pixels == NULL)  return;

    uint32_t x_offset = 0;
    uint32_t y_offset = 0;

    if (dst_surface->width > width) x_offset = (dst_surface->width - width) / 2;

    if (dst_surface->height > height)   y_offset = (dst_surface->height - height) / 2;

    uint32_t dst_y = (uint32_t)scanline + y_offset;

    if (dst_y >= dst_surface->height)   return;

    // Clamp supplied scanline region.
    if (right > width)  right = width;

    if (left >= right)  return;

    // Destination X.
    uint32_t dst_x = x_offset + left;

    if (dst_x >= dst_surface->width)    return;

    // Don't write past the Surface.
    uint32_t pixels_to_copy = (uint32_t)right - left;

    uint32_t max_pixels =   dst_surface->width - dst_x;

    if (pixels_to_copy > max_pixels)    pixels_to_copy = max_pixels;

    if (pixels_to_copy == 0)    return;

    // Calculate the destination row ONCE.
    // SIMD copies the entire contiguous region.
    uint8_t *dst = dst_surface->pixels + dst_y * dst_surface->pitch + dst_x;

    // Source starts at pixels[left].
    const uint8_t *src = pixels + left;

    simd_blit8(
        dst,
        src,
        pixels_to_copy);
}
