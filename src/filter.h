#ifndef FB_FILTER_H
#define FB_FILTER_H

#include <stddef.h>
#include <stdint.h>

/*
 * A filter chain over RGBA8 pixels: the same arithmetic as Surface's NativeFilters, step for step, so the bytes agree.
 * Every step works on premultiplied RGBA8 (straight in, premultiplied by (c * a + 127) / 255; straight out again by
 * (c * 255 + a / 2) / a); a step reads `in` (NULL: the previous result; "source": the pixels) and may name its
 * result. Floats are worked in doubles, built without fused multiply-add (-ffp-contract=off).
 */

typedef enum {
	FB_FILTER_BLUR,
	FB_FILTER_MATRIX,
	FB_FILTER_FLOOD,
	FB_FILTER_COMPOSITE,
	FB_FILTER_BLEND,
	FB_FILTER_NOISE,
	FB_FILTER_DISPLACE,
	FB_FILTER_FIELD,
	FB_FILTER_TABLE
} fb_filter_kind;

#define FB_FILTER_MAX_BANDS 16

typedef struct {
	fb_filter_kind kind;
	const char *in, *in2, *result;

	int sampling;              /* blur: 0 gaussian, 1 spiral */
	int samples;               /* blur: spiral samples 1..64 */
	int edge;                  /* blur: 0 none, 1 duplicate */
	double sx, sy;             /* blur: deviations, or the spiral radius in sx */

	double matrix[20];         /* matrix */

	int flood[4];              /* flood: r, g, b, a bytes */

	int op;                    /* composite: 0 over, in, out, atop, xor, arithmetic; blend: 0 normal, multiply, screen, darken, lighten */
	double k[4];               /* composite arithmetic */

	int noise;                 /* noise: 0 turbulence, 1 fractal, 2 hash */
	double fx, fy;
	int octaves, stitch;
	int32_t seed;

	const uint8_t *map;        /* displace: straight RGBA8, map_w x map_h; or, with `in2`, the map is that earlier result (premultiplied, the layer's size) */
	int map_w, map_h;
	double scale, spread;
	int xchannel, ychannel;    /* 0 r, 1 g, 2 b, 3 a */
	int bands;
	double weights[FB_FILTER_MAX_BANDS * 3];

	int field;                 /* field: 0 rounded box, 1 inside, 2 ramp */
	double box[4], radii[4];   /* field: x0, y0, x1, y1; top left, top right, bottom right, bottom left */
	double from, to;           /* field: the window */

	const uint8_t *tables[4];  /* table: 256 bytes for r, g, b, a, or NULL for none */
} fb_filter_step;

/*
 * Runs `count` steps over `straight` (width x height, straight RGBA8); `origin_x`, `origin_y` are where the top left
 * lies on the surface (noise is a function of it). With `premultiplied` the pixels in are premultiplied already and
 * are the source as they are; the result is straightened either way. Returns the straight RGBA8 result (malloc'd, width * height * 4
 * bytes; the caller frees it), or NULL when memory ran out. Every name a step reads must be "source" or the result
 * of an earlier step: the caller checks that.
 */
uint8_t *fb_filter_run(const fb_filter_step *steps, size_t count, const uint8_t *straight, int width, int height, int origin_x, int origin_y, int premultiplied);

#endif
