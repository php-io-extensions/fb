/*
 * The pixel core: byte layouts, the colour mapper and the loops over them.
 * Plain C over caller-owned memory; nothing here knows PHP.
 *
 * A pixel's word is the same whatever the storage order: 0/1 for mono (1 is
 * lit, white), a grey level or a palette code for index layouts, 0xRGB at 12
 * bits, RGB565 at 16, 0xRRGGBB with the low two bits of each channel zero at
 * 18, 0xRRGGBB at 24, 0xRRGGBBAA at 32, and a mask of inks for planar (0 is
 * paper).
 */

#ifndef PHPFB_CORE_H
#define PHPFB_CORE_H

#include <stddef.h>
#include <stdint.h>

#define FB_MAX_INKS 16
#define FB_MAX_SIDE 65535

enum {
	FB_LAYOUT_MONO_ROWS = 0,   /* 1 bit per pixel, rows padded to a byte */
	FB_LAYOUT_MONO_PAGES = 1,  /* 1 bit per pixel, one byte per column per 8-row page */
	FB_LAYOUT_INDEX2 = 2,      /* 2, 4 or 8 bits per pixel, rows padded to a byte */
	FB_LAYOUT_INDEX4 = 3,
	FB_LAYOUT_INDEX8 = 4,
	FB_LAYOUT_RGB444 = 5,      /* two pixels in three bytes */
	FB_LAYOUT_RGB565 = 6,
	FB_LAYOUT_RGB666 = 7,      /* three bytes, six significant bits left-aligned in each */
	FB_LAYOUT_RGB888 = 8,
	FB_LAYOUT_RGBA8888 = 9,
	FB_LAYOUT_PLANAR = 10      /* one 1-bit plane per ink, each packed like MONO_ROWS */
};

enum { FB_BIT_ORDER_MSB = 0, FB_BIT_ORDER_LSB = 1 };
enum { FB_BYTE_ORDER_MSB = 0, FB_BYTE_ORDER_LSB = 1 };
enum { FB_CHANNELS_RGB = 0, FB_CHANNELS_BGR = 1, FB_CHANNELS_RGBA = 2, FB_CHANNELS_BGRA = 3, FB_CHANNELS_ARGB = 4, FB_CHANNELS_ABGR = 5 };
enum { FB_SCAN_TOP_DOWN = 0, FB_SCAN_BOTTOM_UP = 1 };
enum { FB_MODE_MONO = 0, FB_MODE_GREY = 1, FB_MODE_INDEX = 2, FB_MODE_PLANAR = 3, FB_MODE_RGB = 4 };

typedef struct {
	uint32_t rgb;      /* 0xRRGGBB */
	uint32_t code;     /* the word an index layout stores for this ink */
	uint8_t inverted;  /* a planar plane that stores this ink as 0 */
} fb_ink;

typedef struct {
	int layout;
	int bit_order;
	int byte_order;
	int channel_order;
	int scan;
	int ink_count;
	fb_ink inks[FB_MAX_INKS];
	/* derived by fb_format_resolve() */
	int mode;
	int bits;          /* bits per pixel of an index layout */
	int levels;        /* the highest grey level */
} fb_format;

typedef struct {
	int x;
	int y;
	int width;
	int height;
} fb_rect;

/* Validate a format and derive its mode. NULL when it is usable; otherwise what is wrong with it. */
const char *fb_format_resolve(fb_format *format);

/* True when two formats store a pixel the same way (scan direction aside). */
int fb_format_same_storage(const fb_format *a, const fb_format *b);

size_t fb_size(const fb_format *format, int width, int height);
size_t fb_plane_size(int width, int height);
void fb_granularity(const fb_format *format, int width, int *unit_width, int *unit_height);

/* The fill(0) state: zero bytes, except inverted planar planes, which start at 1. */
void fb_blank(const fb_format *format, uint8_t *bytes, int width, int height);

uint32_t fb_get(const fb_format *format, const uint8_t *bytes, int width, int height, int x, int y);
void fb_set(const fb_format *format, uint8_t *bytes, int width, int height, int x, int y, uint32_t word);
void fb_fill_rect(const fb_format *format, uint8_t *bytes, int width, int height, fb_rect rect, uint32_t word);

uint32_t fb_map(const fb_format *format, int red, int green, int blue, int alpha);
/* A word as 0xRRGGBBAA. */
uint32_t fb_unmap(const fb_format *format, uint32_t word);

/*
 * A rect's pixels into `out` (fb_size(to, rect) bytes) in the `to` format, rows in its scan order.
 * Words are copied as stored when `to` is `format` itself, and mapped through RGBA8 otherwise.
 */
void fb_region(const fb_format *format, const uint8_t *bytes, int width, int height, fb_rect rect, const fb_format *to, uint8_t *out);
void fb_rgba8(const fb_format *format, const uint8_t *bytes, int width, int height, uint8_t *out);

/* Map RGBA8 pixels of a source `source_width` wide, placed at (offset_x, offset_y), into `target`, a rect inside both. */
void fb_blit_rgba8(const fb_format *format, uint8_t *bytes, int width, int height, const uint8_t *rgba8, int source_width, int offset_x, int offset_y, fb_rect target);

/* Copy a rect between two buffers of the same size and storage. */
void fb_copy(const fb_format *format, uint8_t *bytes, const uint8_t *source, int width, int height, fb_rect rect);

/* One bit per pixel into `out` (fb_plane_size bytes), x0 in bit 7: set where the pixel's word equals `word`. */
void fb_plane(const fb_format *format, const uint8_t *bytes, int width, int height, uint32_t word, uint8_t *out);

#define FB_SPAN_BYTES 7

/*
 * Paint `count` spans (FB_SPAN_BYTES each, little-endian y, x, length as uint16,
 * coverage as uint8), every one already checked to lie inside, in 0xRRGGBBAA.
 * Effective alpha is (alpha * coverage + 127) / 255. RGB and grey formats blend
 * source-over through the mapper, (src * a + dst * (255 - a) + 127) / 255 a
 * channel; mono, index and planar formats write the colour where a >= 128.
 */
void fb_paint_spans(const fb_format *format, uint8_t *bytes, int width, int height, const uint8_t *spans, size_t count, uint32_t rgba);

/*
 * Paint RGBA8 pixels (`source_width` x `source_height`, straight alpha) through an
 * inverse placement. For each pixel of `target`, a rect inside the buffer, the point
 * (u, v) = (ia*x + ic*y + ie, ib*x + id*y + if) at the pixel's centre picks the source
 * pixel when it lies inside the image: the one under it, or with `smooth` the four
 * around it (weights in 1/256ths, colours weighed by alpha, edges held). It is blended
 * like a span, with alpha = (source alpha * opacity + 127) / 255. `row` is the row of a
 * larger surface this buffer's row 0 stands for: y counts from there.
 */
void fb_paint_rgba8(const fb_format *format, uint8_t *bytes, int width, int height, const uint8_t *rgba8, int source_width, int source_height,
	const double inverse[6], fb_rect target, int opacity, int smooth, int row);

#endif
