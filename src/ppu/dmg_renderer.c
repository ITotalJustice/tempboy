#include "ppu.h"
#include "../internal.h"
#include "../gb.h"

#include <string.h>
#include <assert.h>


struct DmgPrioBuf
{
    // 0-3
    uint8_t colour_id[GB_SCREEN_WIDTH];
};

static inline uint16_t calculate_col_from_palette(const uint8_t palette, const uint8_t colour)
{
    return ((palette >> (colour << 1)) & 3);
}

static inline void dmg_update_colours(uint32_t colours[4], const uint32_t pal_colours[4], const uint8_t palette, bool* dirty)
{
    if (*dirty)
    {
        *dirty = false;
        
        for (uint8_t i = 0; i < 4; ++i)
        {
            colours[i] = pal_colours[calculate_col_from_palette(palette, i)];
        }
    }
}

void GB_update_all_colours_gb(struct GB_Core* gb)
{
    // the colour palette will be auto updated next frame.
    gb->ppu.dirty_bg[0] = true;
    gb->ppu.dirty_obj[0] = true;
    gb->ppu.dirty_obj[1] = true;
}

struct DMG_SpriteAttribute
{
    bool prio;
    bool yflip;
    bool xflip;
    uint8_t pal; // only 0,1. not set as bool as its not a flag
};

struct DMG_Sprite
{
    int16_t y;
    int16_t x;
    uint8_t i;
    struct DMG_SpriteAttribute a;
};

struct DMG_Sprites
{
    struct DMG_Sprite sprite[10];
    uint8_t count;
};

static FORCE_INLINE struct DMG_SpriteAttribute dmg_get_sprite_attr(const uint8_t v)
{
    return (struct DMG_SpriteAttribute)
    {
        .prio   = (v & 0x80) > 0,
        .yflip  = (v & 0x40) > 0,
        .xflip  = (v & 0x20) > 0,
        .pal    = (v & 0x10) > 0,
    };
}

static inline struct DMG_Sprites dmg_sprite_fetch(const struct GB_Core* gb)
{
    struct DMG_Sprites sprites = {0};
    
    const uint8_t sprite_size = GB_get_sprite_size(gb);
    const uint8_t ly = IO_LY;

    for (size_t i = 0; i < ARRAY_SIZE(gb->ppu.oam); i += 4)
    {
        const int16_t sprite_y = gb->ppu.oam[i + 0] - 16;

        // check if the y is in bounds!
        if (ly >= sprite_y && ly < (sprite_y + sprite_size))
        {
            struct DMG_Sprite* sprite = &sprites.sprite[sprites.count];

            sprite->y = sprite_y;
            sprite->x = gb->ppu.oam[i + 1] - 8;
            sprite->i = gb->ppu.oam[i + 2];
            sprite->a = dmg_get_sprite_attr(gb->ppu.oam[i + 3]);

            ++sprites.count;

            // only 10 sprites per line!
            if (sprites.count == 10)
            {
                break;
            }
        }
    }

    // now we need to sort the spirtes!
    // sprites are ordered by their xpos, however, if xpos match,
    // then the conflicting sprites are sorted based on pos in oam.

    // because the sprites are already sorted via oam index, the following
    // sort preserves the index position.

    if (sprites.count)
    {
        const uint8_t runfor = sprites.count - 1; // silence gcc strict-overflow
        bool unsorted = true;

        while (unsorted)
        {
            unsorted = false;

            for (uint8_t i = 0; i < runfor; ++i)
            {
                if (sprites.sprite[i].x > sprites.sprite[i + 1].x)
                {
                    const struct DMG_Sprite tmp = sprites.sprite[i];
                    
                    sprites.sprite[i] = sprites.sprite[i + 1];
                    sprites.sprite[i + 1] = tmp;

                    unsorted = true;
                }
            }

        }
    }

    return sprites;
}

#define RENDER_BG(pixels, prio_buf) do \
{ \
    const uint8_t scanline = IO_LY; \
    const uint8_t base_tile_x = IO_SCX >> 3; \
    const uint8_t sub_tile_x = (IO_SCX & 7); \
    const uint8_t pixel_y = (scanline + IO_SCY); \
    const uint8_t tile_y = pixel_y >> 3; \
    const uint8_t sub_tile_y = (pixel_y & 7); \
\
    const uint8_t* bit = PIXEL_BIT_SHRINK; \
    /* due how internally the array is represented when NOT built with gbc */ \
    /* support, this needed changing to silence gcc array-bounds */ \
    const uint8_t *vram_map = ((const uint8_t*)gb->ppu.vram) + ((GB_get_bg_map_select(gb) + (tile_y * 32)) & 0x1FFF); \
\
    for (uint8_t tile_x = 0; tile_x <= 20; ++tile_x) \
    { \
        const uint8_t map_x = ((base_tile_x + tile_x) & 31); \
\
        const uint8_t tile_num = vram_map[map_x]; \
        const uint16_t offset = GB_get_tile_offset(gb, tile_num, sub_tile_y); \
\
        const uint8_t byte_a = GB_vram_read(gb, offset + 0, 0); \
        const uint8_t byte_b = GB_vram_read(gb, offset + 1, 0); \
\
        for (uint8_t x = 0; x < 8; ++x) \
        { \
            const int16_t x_index = ((tile_x * 8) + x) - sub_tile_x; \
\
            if (x_index >= GB_SCREEN_WIDTH) \
            { \
                break; \
            } \
\
            if (x_index < 0) \
            { \
                continue; \
            } \
\
            const uint8_t colour_id = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x])); \
\
            prio_buf.colour_id[x_index] = colour_id; \
\
            const uint32_t colour = gb->ppu.bg_colours[0][colour_id]; \
\
            pixels[x_index] = colour; \
        } \
    } \
} while(0)

#define RENDER_WIN(pixels, prio_buf) do \
{ \
    const uint8_t base_tile_x = 20 - (IO_WX >> 3); \
    const int16_t sub_tile_x = IO_WX - 7; \
    const uint8_t pixel_y = gb->ppu.window_line; \
    const uint8_t tile_y = pixel_y >> 3; \
    const uint8_t sub_tile_y = (pixel_y & 7); \
\
    bool did_draw = false; \
\
    const uint8_t* bit = PIXEL_BIT_SHRINK; \
    const uint8_t *vram_map = ((const uint8_t*)gb->ppu.vram) + ((GB_get_win_map_select(gb) + (tile_y * 32)) & 0x1FFF); \
\
    for (uint8_t tile_x = 0; tile_x <= base_tile_x; ++tile_x) \
    { \
        const uint8_t tile_num = vram_map[tile_x]; \
        const uint16_t offset = GB_get_tile_offset(gb, tile_num, sub_tile_y); \
\
        const uint8_t byte_a = GB_vram_read(gb, offset + 0, 0); \
        const uint8_t byte_b = GB_vram_read(gb, offset + 1, 0); \
\
        for (uint8_t x = 0; x < 8; ++x) \
        { \
            const uint8_t x_index = (tile_x * 8) + x + sub_tile_x; \
\
            if (x_index >= GB_SCREEN_WIDTH) \
            { \
                break; \
            } \
\
            did_draw |= true; \
\
            const uint8_t colour_id = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x])); \
\
            prio_buf.colour_id[x_index] = colour_id; \
\
            const uint32_t colour = gb->ppu.bg_colours[0][colour_id]; \
\
            pixels[x_index] = colour; \
        } \
    } \
\
    if (did_draw) \
    { \
        ++gb->ppu.window_line; \
    } \
} while(0)

#define RENDER_OBJ(pixels, prio_buf) do \
{ \
    const uint8_t scanline = IO_LY; \
    const uint8_t sprite_size = GB_get_sprite_size(gb); \
\
    /* keep track of when an oam entry has already been written. */ \
    bool oam_priority[GB_SCREEN_WIDTH] = {0}; \
\
    const struct DMG_Sprites sprites = dmg_sprite_fetch(gb); \
\
    for (uint8_t i = 0; i < sprites.count; ++i) \
    { \
        const struct DMG_Sprite* sprite = &sprites.sprite[i]; \
\
        /* check if the sprite has a chance of being on screen */ \
        /* + 8 because thats the width of each sprite (8 pixels) */ \
        if (sprite->x == -8 || sprite->x >= GB_SCREEN_WIDTH) \
        { \
            continue; \
        } \
\
        const uint8_t sprite_line = sprite->a.yflip ? sprite_size - 1 - (scanline - sprite->y) : scanline - sprite->y; \
        /* when in 8x16 size, bit-0 is ignored of the tile_index */ \
        const uint8_t tile_index = sprite_size == 16 ? sprite->i & 0xFE : sprite->i; \
        const uint16_t offset = (sprite_line << 1) + (tile_index << 4); \
\
        const uint8_t byte_a = GB_vram_read(gb, offset + 0, 0); \
        const uint8_t byte_b = GB_vram_read(gb, offset + 1, 0); \
\
        const uint8_t* bit = sprite->a.xflip ? PIXEL_BIT_GROW : PIXEL_BIT_SHRINK; \
\
        for (int8_t x = 0; x < 8; ++x) \
        { \
            const int16_t x_index = sprite->x + x; \
\
            /* sprite is offscreen, exit loop now */ \
            if (x_index >= GB_SCREEN_WIDTH) \
            { \
                break; \
            } \
\
            /* ensure that we are in bounds */ \
            if (x_index < 0) \
            { \
                continue; \
            } \
\
            if (oam_priority[x_index]) \
            { \
                continue; \
            } \
\
            const uint8_t colour_id = ((!!(byte_b & bit[x])) << 1) | (!!(byte_a & bit[x])); \
\
            if (colour_id == 0) \
            { \
                continue; \
            } \
\
            /* handle prio */ \
            if (sprite->a.prio && prio_buf.colour_id[x_index]) \
            { \
                continue; \
            } \
\
            /* keep track of the sprite pixel that was written (so we don't overlap it!) */ \
            oam_priority[x_index] = true; \
\
            const uint32_t colour = gb->ppu.obj_colours[sprite->a.pal][colour_id]; \
\
            pixels[x_index] = colour; \
        } \
    } \
} while(0)

#define RENDER(bpp, prio_buf) do \
{ \
    uint##bpp##_t* pixels = ((uint##bpp##_t*)gb->pixels) + (gb->stride * IO_LY); \
\
    if (LIKELY(GB_is_bg_enabled(gb))) \
    { \
        RENDER_BG(pixels, prio_buf); \
\
        /* WX=0..166, WY=0..143 */ \
        if ((GB_is_win_enabled(gb)) && (IO_WX <= 166) && (IO_WY <= 143) && (IO_WY <= IO_LY)) \
        { \
            RENDER_WIN(pixels, prio_buf); \
        } \
    } \
\
    if (LIKELY(GB_is_obj_enabled(gb))) \
    { \
        RENDER_OBJ(pixels, prio_buf); \
    } \
} while(0)

void DMG_render_scanline(struct GB_Core* gb)
{
    struct DmgPrioBuf prio_buf = {0};

    // update the DMG colour palettes
    dmg_update_colours(gb->ppu.bg_colours[0], gb->palette.BG, IO_BGP, &gb->ppu.dirty_bg[0]);
    dmg_update_colours(gb->ppu.obj_colours[0], gb->palette.OBJ0, IO_OBP0, &gb->ppu.dirty_obj[0]);
    dmg_update_colours(gb->ppu.obj_colours[1], gb->palette.OBJ1, IO_OBP1, &gb->ppu.dirty_obj[1]);

    switch (gb->bpp)
    {
        case 8:
            RENDER(8, prio_buf);
            break;

        case 15:
        case 16:
            RENDER(16, prio_buf);
            break;

        case 24:
        case 32:
            RENDER(32, prio_buf);
            break;
    }
}

#undef RENDER_BG
#undef RENDER_WIN
#undef RENDER_OBJ
#undef RENDER
