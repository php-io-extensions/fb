#include "filter.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/*
 * Mirrors Surface's NativeFilters line for line: same order of operations, same rounding, so the two agree byte for
 * byte. Build with -ffp-contract=off.
 */

typedef struct {
	int width, height;
	int failed;
	uint8_t **owned;
	size_t owned_count, owned_cap;
	uint8_t *scratch_in, *scratch_out;
} fb_filter_ctx;

static uint8_t *fb_image_new(fb_filter_ctx *ctx)
{
	size_t n = (size_t) ctx->width * (size_t) ctx->height * 4;
	uint8_t *image = ctx->failed ? NULL : malloc(n ? n : 1);

	if (image == NULL) {
		ctx->failed = 1;
		return NULL;
	}
	if (ctx->owned_count == ctx->owned_cap) {
		size_t cap = ctx->owned_cap ? ctx->owned_cap * 2 : 16;
		uint8_t **grown = realloc(ctx->owned, cap * sizeof(uint8_t *));

		if (grown == NULL) {
			free(image);
			ctx->failed = 1;
			return NULL;
		}
		ctx->owned = grown;
		ctx->owned_cap = cap;
	}
	ctx->owned[ctx->owned_count++] = image;
	return image;
}

static inline int fb_premultiply(int channel, int alpha)
{
	return (channel * alpha + 127) / 255;
}

static inline double fb_unit(double v)
{
	return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v);
}

static inline int fb_round(double v)
{
	return (int) floor(v + 0.5);
}

static inline size_t fb_at(int horizontal, int line, int i, int width)
{
	return ((size_t) (horizontal ? line * width + i : i * width + line)) * 4;
}

/* ---------------------------------------------------------------- blur */

static void fb_kernel_axis(fb_filter_ctx *ctx, const uint8_t *in, uint8_t *out, int horizontal, const int *weights, int radius, int edge)
{
	int length = horizontal ? ctx->width : ctx->height, lines = horizontal ? ctx->height : ctx->width;

	for (int line = 0; line < lines; line++) {
		for (int channel = 0; channel < 4; channel++) {
			for (int i = 0; i < length; i++) {
				int64_t sum = 0;

				for (int k = 0; k < 2 * radius + 1; k++) {
					int j = i + k - radius;

					if (j < 0 || j >= length) {
						if (edge == 0) {
							continue;
						}
						j = j < 0 ? 0 : length - 1;
					}
					sum += (int64_t) weights[k] * in[fb_at(horizontal, line, j, ctx->width) + channel];
				}
				out[fb_at(horizontal, line, i, ctx->width) + channel] = (uint8_t) ((sum + 128) >> 8);
			}
		}
	}
}

static inline int fb_box_value(const uint8_t *in, int horizontal, int line, int width, int length, int channel, int edge, int j)
{
	if (j < 0 || j >= length) {
		if (edge == 0) {
			return 0;
		}
		j = j < 0 ? 0 : length - 1;
	}
	return in[fb_at(horizontal, line, j, width) + channel];
}

static void fb_box_axis(fb_filter_ctx *ctx, const uint8_t *in, uint8_t *out, int horizontal, int size, int left, int edge)
{
	int length = horizontal ? ctx->width : ctx->height, lines = horizontal ? ctx->height : ctx->width, half = size / 2;

	for (int line = 0; line < lines; line++) {
		for (int channel = 0; channel < 4; channel++) {
			int64_t sum = 0;

			for (int j = -left; j <= -left + size - 1; j++) {
				sum += fb_box_value(in, horizontal, line, ctx->width, length, channel, edge, j);
			}
			for (int i = 0; i < length; i++) {
				out[fb_at(horizontal, line, i, ctx->width) + channel] = (uint8_t) ((sum + half) / size);
				sum += fb_box_value(in, horizontal, line, ctx->width, length, channel, edge, i - left + size)
					- fb_box_value(in, horizontal, line, ctx->width, length, channel, edge, i - left);
			}
		}
	}
}

static const uint8_t *fb_gaussian_axis(fb_filter_ctx *ctx, const uint8_t *image, int horizontal, double sigma, int edge)
{
	if (sigma <= 0.0) {
		return image;
	}
	if (sigma < 2.0) {
		int radius = (int) ceil(3 * sigma), weights[64];
		double raw[64], total = 0.0;
		int sum = 0;
		uint8_t *out = fb_image_new(ctx);

		if (out == NULL) {
			return NULL;
		}
		for (int i = -radius; i <= radius; i++) {
			raw[i + radius] = exp(-(double) (i * i) / (2 * sigma * sigma));
			total += raw[i + radius];
		}
		for (int i = 0; i < 2 * radius + 1; i++) {
			weights[i] = (int) floor(256 * raw[i] / total + 0.5);
			sum += weights[i];
		}
		weights[radius] += 256 - sum;
		fb_kernel_axis(ctx, image, out, horizontal, weights, radius, edge);
		return out;
	}

	int d = (int) floor(sigma * 3 * sqrt(2 * M_PI) / 4 + 0.5);
	uint8_t *a = fb_image_new(ctx), *b = fb_image_new(ctx), *c = fb_image_new(ctx);

	if (a == NULL || b == NULL || c == NULL) {
		return NULL;
	}
	if (d % 2 == 1) {
		int left = (d - 1) / 2;

		fb_box_axis(ctx, image, a, horizontal, d, left, edge);
		fb_box_axis(ctx, a, b, horizontal, d, left, edge);
		fb_box_axis(ctx, b, c, horizontal, d, left, edge);
		return c;
	}
	fb_box_axis(ctx, image, a, horizontal, d, d / 2, edge);
	fb_box_axis(ctx, a, b, horizontal, d, d / 2 - 1, edge);
	fb_box_axis(ctx, b, c, horizontal, d + 1, d / 2, edge);
	return c;
}

/* The four pixels around (u, v), weights in 1/256ths, rounded to nearest; a pixel off the image is skipped (edge 0) or held. */
static void fb_bilinear(const uint8_t *image, int width, int height, double u, double v, int edge, int out[4])
{
	double fx = u - 0.5, fy = v - 0.5;

	fx = fx < -1.0e9 ? -1.0e9 : (fx > 1.0e9 ? 1.0e9 : fx);
	fy = fy < -1.0e9 ? -1.0e9 : (fy > 1.0e9 ? 1.0e9 : fy);
	int x0 = (int) floor(fx), y0 = (int) floor(fy);
	int tx = (int) floor((fx - x0) * 256), ty = (int) floor((fy - y0) * 256);
	int ys[4] = {y0, y0, y0 + 1, y0 + 1}, xs[4] = {x0, x0 + 1, x0, x0 + 1};
	int weights[4] = {(256 - tx) * (256 - ty), tx * (256 - ty), (256 - tx) * ty, tx * ty};
	int64_t sum[4] = {0, 0, 0, 0};

	for (int n = 0; n < 4; n++) {
		int x = xs[n], y = ys[n];

		if (weights[n] == 0) {
			continue;
		}
		if (x < 0 || x >= width || y < 0 || y >= height) {
			if (edge == 0) {
				continue;
			}
			x = x < 0 ? 0 : (x > width - 1 ? width - 1 : x);
			y = y < 0 ? 0 : (y > height - 1 ? height - 1 : y);
		}
		const uint8_t *p = image + ((size_t) y * (size_t) width + (size_t) x) * 4;

		for (int c = 0; c < 4; c++) {
			sum[c] += (int64_t) weights[n] * p[c];
		}
	}
	for (int c = 0; c < 4; c++) {
		out[c] = (int) ((sum[c] + 32768) >> 16);
	}
}

static uint8_t *fb_spiral(fb_filter_ctx *ctx, const uint8_t *image, double radius, int samples, int edge)
{
	double dx[64], dy[64];
	uint8_t *out = fb_image_new(ctx);

	if (out == NULL) {
		return NULL;
	}
	for (int i = 0; i < samples; i++) {
		double reach = sqrt((i + 0.5) / samples) * radius;

		dx[i] = cos(i * 2.399963229728653) * reach;
		dy[i] = sin(i * 2.399963229728653) * reach;
	}
	for (int y = 0; y < ctx->height; y++) {
		for (int x = 0; x < ctx->width; x++) {
			int64_t sum[4] = {0, 0, 0, 0};
			uint8_t *p = out + ((size_t) y * (size_t) ctx->width + (size_t) x) * 4;

			for (int i = 0; i < samples; i++) {
				int texel[4];

				fb_bilinear(image, ctx->width, ctx->height, x + 0.5 + dx[i], y + 0.5 + dy[i], edge, texel);
				for (int c = 0; c < 4; c++) {
					sum[c] += texel[c];
				}
			}
			for (int c = 0; c < 4; c++) {
				p[c] = (uint8_t) ((sum[c] + samples / 2) / samples);
			}
		}
	}
	return out;
}

static const uint8_t *fb_blur(fb_filter_ctx *ctx, const uint8_t *image, const fb_filter_step *s)
{
	if (s->sampling == 1) {
		return s->sx <= 0.0 ? image : fb_spiral(ctx, image, s->sx, s->samples, s->edge);
	}
	const uint8_t *horizontal = fb_gaussian_axis(ctx, image, 1, s->sx, s->edge);

	return horizontal == NULL ? NULL : fb_gaussian_axis(ctx, horizontal, 0, s->sy, s->edge);
}

/* -------------------------------------------------------- colour steps */

static uint8_t *fb_matrix(fb_filter_ctx *ctx, const uint8_t *image, const double *m)
{
	uint8_t *out = fb_image_new(ctx);
	size_t n = (size_t) ctx->width * (size_t) ctx->height * 4;

	if (out == NULL) {
		return NULL;
	}
	for (size_t at = 0; at < n; at += 4) {
		int a = image[at + 3];
		double r = 0.0, g = 0.0, b = 0.0, alpha = (double) a / 255;

		if (a != 0) {
			int cr = (image[at] * 255 + a / 2) / a, cg = (image[at + 1] * 255 + a / 2) / a, cb = (image[at + 2] * 255 + a / 2) / a;

			r = (double) (cr > 255 ? 255 : cr) / 255;
			g = (double) (cg > 255 ? 255 : cg) / 255;
			b = (double) (cb > 255 ? 255 : cb) / 255;
		}
		double nr = fb_unit(m[0] * r + m[1] * g + m[2] * b + m[3] * alpha + m[4]);
		double ng = fb_unit(m[5] * r + m[6] * g + m[7] * b + m[8] * alpha + m[9]);
		double nb = fb_unit(m[10] * r + m[11] * g + m[12] * b + m[13] * alpha + m[14]);
		double na = fb_unit(m[15] * r + m[16] * g + m[17] * b + m[18] * alpha + m[19]);

		out[at] = (uint8_t) fb_round(nr * na * 255);
		out[at + 1] = (uint8_t) fb_round(ng * na * 255);
		out[at + 2] = (uint8_t) fb_round(nb * na * 255);
		out[at + 3] = (uint8_t) fb_round(na * 255);
	}
	return out;
}

static uint8_t *fb_flood(fb_filter_ctx *ctx, const int color[4])
{
	uint8_t *out = fb_image_new(ctx);
	size_t n = (size_t) ctx->width * (size_t) ctx->height * 4;

	if (out == NULL) {
		return NULL;
	}
	for (size_t at = 0; at < n; at += 4) {
		out[at] = (uint8_t) fb_premultiply(color[0], color[3]);
		out[at + 1] = (uint8_t) fb_premultiply(color[1], color[3]);
		out[at + 2] = (uint8_t) fb_premultiply(color[2], color[3]);
		out[at + 3] = (uint8_t) color[3];
	}
	return out;
}

/* A premultiplied pixel of 0..1 floats: clamped, the colours held to the alpha, rounded to nearest. */
static inline void fb_store(uint8_t *out, const double p[4])
{
	int alpha = fb_round(fb_unit(p[3]) * 255);

	for (int c = 0; c < 3; c++) {
		int v = fb_round(fb_unit(p[c]) * 255);

		out[c] = (uint8_t) (v < alpha ? v : alpha);
	}
	out[3] = (uint8_t) alpha;
}

static uint8_t *fb_composite(fb_filter_ctx *ctx, const uint8_t *a, const uint8_t *b, const fb_filter_step *s)
{
	uint8_t *out = fb_image_new(ctx);
	size_t n = (size_t) ctx->width * (size_t) ctx->height * 4;

	if (out == NULL) {
		return NULL;
	}
	for (size_t at = 0; at < n; at += 4) {
		double aa = (double) a[at + 3] / 255, ab = (double) b[at + 3] / 255, p[4];

		for (int c = 0; c < 4; c++) {
			double x = (double) a[at + c] / 255, y = (double) b[at + c] / 255;

			switch (s->op) {
				case 0: p[c] = x + y * (1 - aa); break;
				case 1: p[c] = x * ab; break;
				case 2: p[c] = x * (1 - ab); break;
				case 3: p[c] = x * ab + y * (1 - aa); break;
				case 4: p[c] = x * (1 - ab) + y * (1 - aa); break;
				default: p[c] = s->k[0] * x * y + s->k[1] * x + s->k[2] * y + s->k[3]; break;
			}
		}
		fb_store(out + at, p);
	}
	return out;
}

static uint8_t *fb_blend_images(fb_filter_ctx *ctx, const uint8_t *a, const uint8_t *b, int mode)
{
	uint8_t *out = fb_image_new(ctx);
	size_t n = (size_t) ctx->width * (size_t) ctx->height * 4;

	if (out == NULL) {
		return NULL;
	}
	for (size_t at = 0; at < n; at += 4) {
		double qa = (double) a[at + 3] / 255, qb = (double) b[at + 3] / 255, p[4];

		for (int c = 0; c < 3; c++) {
			double ca = (double) a[at + c] / 255, cb = (double) b[at + c] / 255;

			switch (mode) {
				case 0: p[c] = (1 - qa) * cb + ca; break;
				case 1: p[c] = (1 - qa) * cb + (1 - qb) * ca + ca * cb; break;
				case 2: p[c] = cb + ca - ca * cb; break;
				case 3: {
					double x = (1 - qa) * cb + ca, y = (1 - qb) * ca + cb;

					p[c] = x < y ? x : y;
					break;
				}
				default: {
					double x = (1 - qa) * cb + ca, y = (1 - qb) * ca + cb;

					p[c] = x > y ? x : y;
					break;
				}
			}
		}
		p[3] = 1 - (1 - qa) * (1 - qb);
		fb_store(out + at, p);
	}
	return out;
}

/* --------------------------------------------------------------- noise */

typedef struct {
	int selector[514];
	double gradient[4][514][2];
} fb_lattice;

static void fb_lattice_init(fb_lattice *lattice, int32_t seed)
{
	const int64_t size = 0x100, modulus = 2147483647;
	int64_t value = seed;

	if (value <= 0) {
		value = -(value % (modulus - 1)) + 1;
	}
	if (value > modulus - 1) {
		value = modulus - 1;
	}
#define FB_RANDOM(into) do { \
		int64_t r_ = 16807 * (value % 127773) - 2836 * (value / 127773); \
		if (r_ <= 0) { r_ += modulus; } \
		value = r_; \
		(into) = r_; \
	} while (0)
	for (int k = 0; k < 4; k++) {
		for (int i = 0; i < size; i++) {
			double pair[2], length;
			int64_t r;

			lattice->selector[i] = i;
			for (int j = 0; j < 2; j++) {
				FB_RANDOM(r);
				pair[j] = (double) ((r % (size + size)) - size) / size;
			}
			length = sqrt(pair[0] * pair[0] + pair[1] * pair[1]);
			lattice->gradient[k][i][0] = length == 0.0 ? 0.0 : pair[0] / length;
			lattice->gradient[k][i][1] = length == 0.0 ? 0.0 : pair[1] / length;
		}
	}
	for (int i = size - 1; i > 0; i--) {
		int k = lattice->selector[i];
		int64_t r;

		FB_RANDOM(r);
		int j = (int) (r % size);
		lattice->selector[i] = lattice->selector[j];
		lattice->selector[j] = k;
	}
#undef FB_RANDOM
	for (int i = 0; i < size + 2; i++) {
		lattice->selector[size + i] = lattice->selector[i];
		for (int k = 0; k < 4; k++) {
			lattice->gradient[k][size + i][0] = lattice->gradient[k][i][0];
			lattice->gradient[k][size + i][1] = lattice->gradient[k][i][1];
		}
	}
}

static double fb_noise2(const fb_lattice *lattice, int channel, double x, double y, const int64_t *stitch)
{
	double t = x + 4096;
	int64_t bx0 = ((int64_t) t) & 0xFF, bx1;
	double rx0 = t - (int64_t) t, rx1 = rx0 - 1.0;
	int64_t by0, by1;
	double ry0, ry1;

	bx1 = (bx0 + 1) & 0xFF;
	t = y + 4096;
	by0 = ((int64_t) t) & 0xFF;
	by1 = (by0 + 1) & 0xFF;
	ry0 = t - (int64_t) t;
	ry1 = ry0 - 1.0;
	if (stitch != NULL) {
		if (bx0 >= stitch[2]) { bx0 -= stitch[0]; }
		if (bx1 >= stitch[2]) { bx1 -= stitch[0]; }
		if (by0 >= stitch[3]) { by0 -= stitch[1]; }
		if (by1 >= stitch[3]) { by1 -= stitch[1]; }
	}
	bx0 &= 0xFF;
	bx1 &= 0xFF;
	by0 &= 0xFF;
	by1 &= 0xFF;
	int i = lattice->selector[bx0], j = lattice->selector[bx1];
	int b00 = lattice->selector[i + by0], b10 = lattice->selector[j + by0];
	int b01 = lattice->selector[i + by1], b11 = lattice->selector[j + by1];
	double sx = rx0 * rx0 * (3.0 - 2.0 * rx0), sy = ry0 * ry0 * (3.0 - 2.0 * ry0);
	const double (*g)[2] = lattice->gradient[channel];
	double u = rx0 * g[b00][0] + ry0 * g[b00][1];
	double v = rx1 * g[b10][0] + ry0 * g[b10][1];
	double a = u + sx * (v - u);

	u = rx0 * g[b01][0] + ry1 * g[b01][1];
	v = rx1 * g[b11][0] + ry1 * g[b11][1];
	double b = u + sx * (v - u);

	return a + sy * (b - a);
}

static double fb_turbulence(const fb_lattice *lattice, int channel, int px, int py, const fb_filter_step *s, int tile_x, int tile_y, int tile_w, int tile_h)
{
	double fx = s->fx, fy = s->fy, sum = 0.0, ratio = 1.0;
	int64_t stitch[4], *use = NULL;

	if (s->stitch) {
		if (fx != 0.0) {
			double low = floor(tile_w * fx) / tile_w, high = ceil(tile_w * fx) / tile_w;

			fx = low > 0.0 && fx / low < high / fx ? low : high;
		}
		if (fy != 0.0) {
			double low = floor(tile_h * fy) / tile_h, high = ceil(tile_h * fy) / tile_h;

			fy = low > 0.0 && fy / low < high / fy ? low : high;
		}
		int64_t w = (int64_t) (tile_w * fx + 0.5), h = (int64_t) (tile_h * fy + 0.5);

		stitch[0] = w;
		stitch[1] = h;
		stitch[2] = (int64_t) (tile_x * fx + 4096 + w);
		stitch[3] = (int64_t) (tile_y * fy + 4096 + h);
		use = stitch;
	}
	double vx = px * fx, vy = py * fy;

	for (int octave = 0; octave < s->octaves; octave++) {
		double n = fb_noise2(lattice, channel, vx, vy, use);

		sum += (s->noise == 1 ? n : fabs(n)) / ratio;
		vx *= 2;
		vy *= 2;
		ratio *= 2;
		if (use != NULL) {
			stitch[0] *= 2;
			stitch[1] *= 2;
			stitch[2] = 2 * stitch[2] - 4096;
			stitch[3] = 2 * stitch[3] - 4096;
		}
	}
	return sum;
}

static uint8_t *fb_noise(fb_filter_ctx *ctx, const fb_filter_step *s, int origin_x, int origin_y)
{
	uint8_t *out = fb_image_new(ctx);

	if (out == NULL) {
		return NULL;
	}
	if (s->noise == 2) {
		for (int y = 0; y < ctx->height; y++) {
			for (int x = 0; x < ctx->width; x++) {
				uint64_t h = ((uint64_t) (int64_t) (x + origin_x) * 374761393u + (uint64_t) (int64_t) (y + origin_y) * 668265263u + (uint64_t) (int64_t) s->seed * 2246822519u) & 0xFFFFFFFFu;
				uint8_t *p = out + ((size_t) y * (size_t) ctx->width + (size_t) x) * 4;

				h = ((h ^ (h >> 13)) * 1274126177u) & 0xFFFFFFFFu;
				p[0] = p[1] = p[2] = (uint8_t) ((h ^ (h >> 16)) & 0xFF);
				p[3] = 255;
			}
		}
		return out;
	}
	fb_lattice *lattice = malloc(sizeof(fb_lattice));

	if (lattice == NULL) {
		ctx->failed = 1;
		return NULL;
	}
	fb_lattice_init(lattice, s->seed);
	for (int y = 0; y < ctx->height; y++) {
		for (int x = 0; x < ctx->width; x++) {
			int straight[4];
			uint8_t *p = out + ((size_t) y * (size_t) ctx->width + (size_t) x) * 4;

			for (int channel = 0; channel < 4; channel++) {
				double sum = fb_turbulence(lattice, channel, x + origin_x, y + origin_y, s, origin_x, origin_y, ctx->width, ctx->height);
				double value = s->noise == 1 ? (sum * 255 + 255) / 2 : sum * 255;
				int v = (int) floor(value + 0.5);

				straight[channel] = v < 0 ? 0 : (v > 255 ? 255 : v);
			}
			p[0] = (uint8_t) fb_premultiply(straight[0], straight[3]);
			p[1] = (uint8_t) fb_premultiply(straight[1], straight[3]);
			p[2] = (uint8_t) fb_premultiply(straight[2], straight[3]);
			p[3] = (uint8_t) straight[3];
		}
	}
	free(lattice);
	return out;
}

/* -------------------------------------------------------- displacement */

static uint8_t *fb_displace(fb_filter_ctx *ctx, const uint8_t *image, const fb_filter_step *s, const uint8_t *mapimg)
{
	uint8_t *out = fb_image_new(ctx);

	if (out == NULL) {
		return NULL;
	}
	memcpy(out, image, (size_t) ctx->width * (size_t) ctx->height * 4);
	for (int y = 0; y < ctx->height; y++) {
		for (int x = 0; x < ctx->width; x++) {
			double dx = 0.0, dy = 0.0;
			uint8_t *p = out + ((size_t) y * (size_t) ctx->width + (size_t) x) * 4;
			int t[4];

			if (mapimg != NULL) {
				/* a map an earlier step made: premultiplied there, straight here */
				const uint8_t *m = mapimg + ((size_t) y * (size_t) ctx->width + (size_t) x) * 4;
				int a = m[3], v[4];

				for (int c = 0; c < 3; c++) {
					int q = a == 0 ? 0 : (m[c] * 255 + a / 2) / a;

					v[c] = q > 255 ? 255 : q;
				}
				v[3] = a;
				dx = s->scale * ((double) v[s->xchannel] / 255 - 0.5);
				dy = s->scale * ((double) v[s->ychannel] / 255 - 0.5);
			} else if (x < s->map_w && y < s->map_h) {
				const uint8_t *m = s->map + ((size_t) y * (size_t) s->map_w + (size_t) x) * 4;

				dx = s->scale * ((double) m[s->xchannel] / 255 - 0.5);
				dy = s->scale * ((double) m[s->ychannel] / 255 - 0.5);
			}
			if (s->bands == 1) {
				fb_bilinear(image, ctx->width, ctx->height, x + 0.5 + dx, y + 0.5 + dy, 0, t);
				p[0] = (uint8_t) t[0];
				p[1] = (uint8_t) t[1];
				p[2] = (uint8_t) t[2];
				p[3] = (uint8_t) t[3];
				continue;
			}
			double rgb[3] = {0.0, 0.0, 0.0};

			for (int i = 0; i < s->bands; i++) {
				double k = (double) i / (s->bands - 1) * 2 - 1, f = 1 + s->spread * k;

				fb_bilinear(image, ctx->width, ctx->height, x + 0.5 + dx * f, y + 0.5 + dy * f, 0, t);
				rgb[0] += t[0] * s->weights[i * 3];
				rgb[1] += t[1] * s->weights[i * 3 + 1];
				rgb[2] += t[2] * s->weights[i * 3 + 2];
			}
			fb_bilinear(image, ctx->width, ctx->height, x + 0.5 + dx, y + 0.5 + dy, 0, t);
			int alpha = t[3];

			for (int c = 0; c < 3; c++) {
				int v = (int) floor(rgb[c] + 0.5);

				p[c] = (uint8_t) (v < alpha ? v : alpha);
			}
			p[3] = (uint8_t) alpha;
		}
	}
	return out;
}

/* ----------------------------------------------------------------- run */

/* -------------------------------------------------------- field and table */

/* Raven's sd_round_box_normal (ui.frag:42-57): the distance to the rounded box (negative inside) and the direction out of it. */
static void fb_box_distance(double px, double py, const double box[4], const double radii[4], double out[3])
{
	double hx = (box[2] - box[0]) / 2, hy = (box[3] - box[1]) / 2;
	double p0 = px - (box[0] + box[2]) / 2, p1 = py - (box[1] + box[3]) / 2;
	double radius = p0 < 0.0 ? (p1 < 0.0 ? radii[0] : radii[3]) : (p1 < 0.0 ? radii[1] : radii[2]);
	double side_x = p0 < 0.0 ? -1.0 : 1.0, side_y = p1 < 0.0 ? -1.0 : 1.0;
	double qx, qy, corner, nx, ny;

	radius = radius < hx ? radius : hx;
	radius = radius < hy ? radius : hy;
	qx = fabs(p0) - hx + radius;
	qy = fabs(p1) - hy + radius;
	corner = hypot(qx > 0.0 ? qx : 0.0, qy > 0.0 ? qy : 0.0);
	if (qx > 0.0 && qy > 0.0) {
		nx = side_x * (qx / (corner > 1e-4 ? corner : 1e-4));
		ny = side_y * (qy / (corner > 1e-4 ? corner : 1e-4));
	} else if (qx > qy) {
		nx = side_x;
		ny = 0.0;
	} else {
		nx = 0.0;
		ny = side_y;
	}
	out[0] = ((qx > qy ? qx : qy) < 0.0 ? (qx > qy ? qx : qy) : 0.0) + corner - radius;
	out[1] = nx;
	out[2] = ny;
}

static inline int fb_unit_byte(double v)
{
	return (int) floor(fb_unit(v) * 255 + 0.5);
}

static uint8_t *fb_field(fb_filter_ctx *ctx, const fb_filter_step *s, int origin_x, int origin_y)
{
	uint8_t *out = fb_image_new(ctx);
	double span = s->to - s->from;

	if (out == NULL) {
		return NULL;
	}
	for (int y = 0; y < ctx->height; y++) {
		for (int x = 0; x < ctx->width; x++) {
			uint8_t *p = out + ((size_t) y * (size_t) ctx->width + (size_t) x) * 4;
			double px = x + origin_x + 0.5, py = y + origin_y + 0.5, f[3];

			p[3] = 255;
			if (s->field == 2) {
				p[0] = p[1] = p[2] = (uint8_t) fb_unit_byte((py - s->from) / span);
				continue;
			}
			fb_box_distance(px, py, s->box, s->radii, f);
			if (s->field == 1) {
				p[0] = p[1] = p[2] = f[0] < 0.0 ? 255 : 0;
				continue;
			}
			p[0] = (uint8_t) fb_unit_byte((f[0] - s->from) / span);
			p[1] = (uint8_t) fb_unit_byte(0.5 + 0.5 * f[1]);
			p[2] = (uint8_t) fb_unit_byte(0.5 + 0.5 * f[2]);
		}
	}
	return out;
}

static uint8_t *fb_table(fb_filter_ctx *ctx, const uint8_t *image, const fb_filter_step *s)
{
	uint8_t *out = fb_image_new(ctx);
	size_t n = (size_t) ctx->width * (size_t) ctx->height * 4;

	if (out == NULL) {
		return NULL;
	}
	for (size_t at = 0; at < n; at += 4) {
		int a = image[at + 3], straight[3] = {0, 0, 0}, na = s->tables[3] ? s->tables[3][a] : a;

		if (a != 0) {
			for (int c = 0; c < 3; c++) {
				int v = (image[at + c] * 255 + a / 2) / a;

				straight[c] = v > 255 ? 255 : v;
			}
		}
		for (int c = 0; c < 3; c++) {
			out[at + c] = (uint8_t) fb_premultiply(s->tables[c] ? s->tables[c][straight[c]] : straight[c], na);
		}
		out[at + 3] = (uint8_t) na;
	}
	return out;
}

static const uint8_t *fb_input(const fb_filter_step *steps, const uint8_t **results, size_t before, const char *name, const uint8_t *previous, const uint8_t *source)
{
	if (name == NULL || name[0] == '\0') {
		return previous;
	}
	if (strcmp(name, "source") == 0) {
		/* a step may be named "source" too, as in SVG: the later definition wins */
		for (size_t i = before; i-- > 0;) {
			if (steps[i].result != NULL && strcmp(steps[i].result, "source") == 0) {
				return results[i];
			}
		}
		return source;
	}
	for (size_t i = before; i-- > 0;) {
		if (steps[i].result != NULL && strcmp(steps[i].result, name) == 0) {
			return results[i];
		}
	}
	return NULL;
}

uint8_t *fb_filter_run(const fb_filter_step *steps, size_t count, const uint8_t *straight, int width, int height, int origin_x, int origin_y, int premultiplied)
{
	fb_filter_ctx ctx = {width, height, 0, NULL, 0, 0, NULL, NULL};
	size_t n = (size_t) width * (size_t) height * 4;
	uint8_t *source = fb_image_new(&ctx), *final = NULL;
	const uint8_t **results = calloc(count ? count : 1, sizeof(uint8_t *));
	const uint8_t *previous;

	if (source == NULL || results == NULL) {
		goto done;
	}
	for (size_t at = 0; at < n; at += 4) {
		int a = straight[at + 3];

		source[at] = (uint8_t) (premultiplied ? straight[at] : fb_premultiply(straight[at], a));
		source[at + 1] = (uint8_t) (premultiplied ? straight[at + 1] : fb_premultiply(straight[at + 1], a));
		source[at + 2] = (uint8_t) (premultiplied ? straight[at + 2] : fb_premultiply(straight[at + 2], a));
		source[at + 3] = (uint8_t) a;
	}
	previous = source;
	for (size_t i = 0; i < count && !ctx.failed; i++) {
		const fb_filter_step *s = &steps[i];
		const uint8_t *in = fb_input(steps, results, i, s->in, previous, source), *in2 = NULL, *made = NULL;

		if (s->kind == FB_FILTER_COMPOSITE || s->kind == FB_FILTER_BLEND || (s->kind == FB_FILTER_DISPLACE && s->in2 != NULL)) {
			in2 = fb_input(steps, results, i, s->in2, previous, source);
		}
		if (in == NULL && s->kind != FB_FILTER_FLOOD && s->kind != FB_FILTER_NOISE && s->kind != FB_FILTER_FIELD) {
			ctx.failed = 1;
			break;
		}
		switch (s->kind) {
			case FB_FILTER_BLUR: made = fb_blur(&ctx, in, s); break;
			case FB_FILTER_MATRIX: made = fb_matrix(&ctx, in, s->matrix); break;
			case FB_FILTER_FLOOD: made = fb_flood(&ctx, s->flood); break;
			case FB_FILTER_COMPOSITE: made = in2 == NULL ? NULL : fb_composite(&ctx, in, in2, s); break;
			case FB_FILTER_BLEND: made = in2 == NULL ? NULL : fb_blend_images(&ctx, in, in2, s->op); break;
			case FB_FILTER_NOISE: made = fb_noise(&ctx, s, origin_x, origin_y); break;
			case FB_FILTER_DISPLACE: made = s->in2 != NULL && in2 == NULL ? NULL : fb_displace(&ctx, in, s, s->in2 != NULL ? in2 : NULL); break;
			case FB_FILTER_FIELD: made = fb_field(&ctx, s, origin_x, origin_y); break;
			case FB_FILTER_TABLE: made = fb_table(&ctx, in, s); break;
		}
		if (made == NULL) {
			ctx.failed = 1;
			break;
		}
		results[i] = made;
		previous = made;
	}
	if (ctx.failed) {
		goto done;
	}
	final = malloc(n ? n : 1);
	if (final == NULL) {
		goto done;
	}
	for (size_t at = 0; at < n; at += 4) {
		int a = previous[at + 3];

		if (a == 0) {
			final[at] = final[at + 1] = final[at + 2] = final[at + 3] = 0;
			continue;
		}
		for (int c = 0; c < 3; c++) {
			int v = (previous[at + c] * 255 + a / 2) / a;

			final[at + c] = (uint8_t) (v > 255 ? 255 : v);
		}
		final[at + 3] = (uint8_t) a;
	}
done:
	for (size_t i = 0; i < ctx.owned_count; i++) {
		free(ctx.owned[i]);
	}
	free(ctx.owned);
	free(results);
	return final;
}
