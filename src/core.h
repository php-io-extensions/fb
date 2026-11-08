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

/* A coverage mask placed on the buffer: `width` x `height` bytes, rows top-down, its top-left pixel at (left, top). */
typedef struct {
	const uint8_t *coverage;
	int width;
	int height;
	int left;
	int top;
} fb_mask;

/*
 * Paint `count` masks in 0xRRGGBBAA, in order, each cut to `clip` (a rect inside the buffer). Each pixel of coverage
 * c > 0 is painted as a span of coverage c is: the same alpha, blend and 128 rule as fb_paint_spans. Returns 1 and
 * sets `bounds` to the box of the pixels with coverage (the box spans of them would have), 0 when there are none.
 */
int fb_paint_masks(const fb_format *format, uint8_t *bytes, int width, int height, const fb_mask *masks, size_t count, uint32_t rgba, fb_rect clip, fb_rect *bounds);

/*
 * Paint a gradient through `count` spans (FB_SPAN_BYTES each, y, x, length uint16 LE, coverage uint8), every one
 * already checked to lie inside. `ramp` is 256 colours of 4 bytes, RGBA, straight alpha. The point (u, v) =
 * (ia*x + ic*y + ie, ib*x + id*y + if) at a pixel's centre (y counted from `row`, a larger surface's row for this
 * buffer's row 0) gives t = u, or hypot(u, v) when `radial`; `spread` 0 pad, 1 repeat, 2 reflect folds t into 0..1;
 * the colour is ramp[floor(t * 255 + 0.5)]. u and v are worked out at a span's first pixel and then stepped by ia and
 * ib, so a pixel costs two additions. Blended like a span, alpha = (ramp alpha * coverage + 127) / 255.
 */
void fb_paint_gradient(const fb_format *format, uint8_t *bytes, int width, int height, const uint8_t *spans, size_t count,
	const uint8_t *ramp, int radial, int spread, const double inverse[6], int row);

/*
 * Paint RGBA8 pixels (`source_width` x `source_height`, straight alpha) through an
 * inverse placement. For each pixel of `target`, a rect inside the buffer, the point
 * (u, v) = (ia*x + ic*y + ie, ib*x + id*y + if) at the pixel's centre picks the source
 * pixel when it lies inside the image: the one under it, or with `smooth` the four
 * around it (weights in 1/256ths, colours weighed by alpha, edges held). It is blended
 * like a span, with alpha = (source alpha * opacity + 127) / 255. `row` is the row of a
 * larger surface this buffer's row 0 stands for: y counts from there.
 *
 * Three more parts change each sampled pixel, in this order, each (x * y + 127) / 255:
 * `tint` (0xRRGGBBAA) is multiplied into every channel, alpha too; then the opacity; then,
 * with a mask, the coverage of the span that holds the pixel (a pixel no span holds is
 * skipped). `window` (NULL: none) is {u0, v0, u1, v1}: only pixels whose point lies inside
 * it are painted; smooth sampling still reads the whole image. The mask is `mask_count`
 * 7-byte spans (y, x, length uint16 LE, coverage), rows ascending and x ascending within a
 * row without overlap, every span inside the buffer, in the buffer's own rows.
 */
void fb_paint_rgba8(const fb_format *format, uint8_t *bytes, int width, int height, const uint8_t *rgba8, int source_width, int source_height,
	const double inverse[6], fb_rect target, int opacity, int smooth, int row, uint32_t tint, const double *window,
	const uint8_t *mask, size_t mask_count);

/*
 * Glass (Surface's drawing-path spec, G4): a dual-filter (Kawase) pyramid over RGBA8 levels, and one pane composited
 * from it. Levels are RGBA8888 bytes, 4 a pixel, rows packed; every operation is per byte, so any channel order works.
 *
 * Down: `dst` (dw x dh, each ceil of the region's half) from the region `r` of `src` (sw wide): out(i, j) =
 * (4 * C + S + 16) >> 5, C the sum of the 2x2 block at (2i, 2j) of the region and S the sum of the 4x4 block at
 * (2i - 1, 2j - 1): the centre's bilinear tap and the four corner taps at +/-1 source pixel. Reads past the region
 * take its edge.
 * Up: `dst` (dw x dh, at most twice `src`) from `src` (sw x sh): the 8-tap tent, the edge midpoints at +/-1 source
 * pixel (weight 1) and the diagonals at +/-1/2 (weight 2), each tap bilinear in quarter pixels, (sum + 96) / 192.
 * Reads past the source take its edge.
 */
void fb_kawase_down(const uint8_t *src, int sw, fb_rect r, uint8_t *dst, int dw, int dh);
void fb_kawase_up(const uint8_t *src, int sw, int sh, uint8_t *dst, int dw, int dh);

/* A glass pane: its rounded box, the pyramid level it samples and the sharp capture, and its look. */
typedef struct {
	double box[4];             /* x0, y0, x1, y1 in target pixels */
	double radii[4];           /* tl, tr, br, bl, held to 0..min(w, h)/2 */
	const uint8_t *blur;       /* the blurred level, RGBA bytes */
	int blur_width, blur_height, scale;
	const uint8_t *sharp;      /* the capture at full size, RGBA bytes; NULL when frost is 1 */
	int sharp_width, sharp_height;
	int origin_x, origin_y;    /* the target pixel both levels start at */
	double frost, refraction, bevel, saturation, light, grain;
	uint32_t tint, rim;        /* 0xRRGGBBAA */
	int alpha;                 /* 0..255, the pane's opacity */
} fb_glass;

/*
 * One pane, every pixel of its box inside `clip` (inside the buffer) once, in doubles: d the signed distance to the
 * rounded box at the pixel's centre and n its outward normal; nothing where d >= 1/2; f = e * e with
 * e = clamp(1 + d / band, 0, 1), band the bevel (else the refraction, else 1); the sample point moved by
 * -n * refraction * f; the blurred level sampled there bilinearly (and the sharp capture, mixed by frost);
 * saturation about luma 0.2126/0.7152/0.0722; the tint mixed in by its alpha; + light * 255; + rim * rim alpha * f *
 * (1/2 + n . (-0.6, -0.8) / 2); + grain * 0.04 * (hash(x, y) - 1/2) * 255; each channel floor(c + 1/2) held to
 * 0..255, blended like a span at floor(clamp(1/2 - d, 0, 1) * alpha + 1/2). 0 when nothing was painted, else 1 with
 * the bounds of what was.
 */
int fb_paint_glass(const fb_format *format, uint8_t *bytes, int width, int height, const fb_glass *glass, fb_rect clip, fb_rect *bounds);

/* The grain hash of a pixel: 0..65535, the same everywhere. */
uint32_t fb_grain(int x, int y);

#endif
