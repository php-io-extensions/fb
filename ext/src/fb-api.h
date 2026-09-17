#ifndef FB_API_H
#define FB_API_H

#include "php.h"

/* kinds */
#define FB_KIND_FULL   0
#define FB_KIND_DIRTY  1
#define FB_KIND_EPAPER 2
#define FB_KIND_PAGED  3
#define FB_KIND_RING   4

/* layouts, chosen at create from (pixel_format, bit_depth, page_axis) */
#define FB_L_MONO_H    0
#define FB_L_MONO_VP   1
#define FB_L_INDEX     2
#define FB_L_RGB444    3
#define FB_L_RGB565    4
#define FB_L_RGB666    5
#define FB_L_RGB888    6
#define FB_L_RGBA8888  7
#define FB_L_PLANAR    8

/* mapper modes */
#define FB_M_MONO   0
#define FB_M_GREY   1
#define FB_M_PLANAR 2
#define FB_M_INDEX  3
#define FB_M_RGB    4

#define FB_MAX_PALETTE 16
#define FB_MAX_DAMAGE  16

typedef struct {
    int x, y, w, h;
} fb_rect;

typedef struct {
    int      n;
    uint32_t rgb[FB_MAX_PALETTE];
    int      inverted[FB_MAX_PALETTE];
    int      code[FB_MAX_PALETTE];
} fb_palette;

typedef struct {
    int pixel_format, bit_depth, bit_order, endianness, page_axis, scan;
    int layout, mode, bits, planes;
    fb_palette palette;
} fb_format;

typedef struct fb_buffer {
    int       kind;
    fb_format fmt;
    int       width, height;      /* virtual surface */
    int       store_w, store_h;   /* the bytes' own size: page window for paged, else w x h */
    size_t    size;               /* bytes per store */
    unsigned char *bytes;         /* full/dirty/epaper: the store; paged: the window; ring: unused (see ring) */
    /* dirty */
    fb_rect   damage[FB_MAX_DAMAGE];
    int       damage_n;
    int       collapsed;
    /* paged */
    int       page_rows, page, pages;
    /* ring */
    int       frames, front, back;
    unsigned char **ring;
} fb_buffer;

/* entry points, one per Fb\Buffer method; zval-in/zval-out like posi's src/*-api.c */
zend_long fb_api_create(zval *kind, zval *pixel_format, zval *bit_depth, zval *bit_order, zval *endianness, zval *page_axis, zval *scan, zval *palette, zval *width, zval *height, zval *extra);
void      fb_api_free(zval *h);
zend_long fb_api_width(zval *h);
zend_long fb_api_height(zval *h);
zend_long fb_api_size(zval *h);
zend_long fb_api_get(zval *h, zval *x, zval *y);
void      fb_api_set(zval *h, zval *x, zval *y, zval *v);
void      fb_api_set_pixels(zval *h, zval *pixels);
void      fb_api_set_segment(zval *h, zval *x, zval *y, zval *w, zval *hh, zval *v);
void      fb_api_fill(zval *h, zval *v);
void      fb_api_bytes(zval *return_value, zval *h);
void      fb_api_region(zval *return_value, zval *h, zval *x, zval *y, zval *w, zval *hh, zval *scan);
void      fb_api_layer(zval *return_value, zval *h, zval *layer);
zend_long fb_api_pointer(zval *h);
void      fb_api_to_rgba8(zval *return_value, zval *h);
void      fb_api_blit_rgba8(zval *h, zval *rgba8, zval *sw, zval *sh, zval *ox, zval *oy);
void      fb_api_transcode(zval *return_value, zval *h, zval *pixel_format, zval *bit_depth, zval *bit_order, zval *endianness, zval *page_axis, zval *scan, zval *palette, zval *x, zval *y, zval *w, zval *hh);
void      fb_api_granularity(zval *return_value, zval *h);
void      fb_api_begin_epoch(zval *h);
void      fb_api_damage(zval *return_value, zval *h);
void      fb_api_set_page(zval *h, zval *page);
zend_long fb_api_page(zval *h);
zend_long fb_api_pages(zval *h);
void      fb_api_present(zval *h);
zend_long fb_api_frames(zval *h);

#endif
