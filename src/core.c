#include "core.h"

#include <math.h>
#include <string.h>

/* Rec. 709 luma weights scaled by 10000: luma(255, 255, 255) is 2 550 000. */
#define FB_LUMA_MAX 2550000u

/* Byte offsets of red, green, blue and alpha within a 32-bit pixel, by channel order. */
static const uint8_t fb_rgba_at[6][4] = {
	{0, 1, 2, 3}, {0, 1, 2, 3},
	{0, 1, 2, 3},  /* RGBA */
	{2, 1, 0, 3},  /* BGRA */
	{1, 2, 3, 0},  /* ARGB */
	{3, 2, 1, 0},  /* ABGR */
};

const char *fb_format_resolve(fb_format *f)
{
	if (f->layout < FB_LAYOUT_MONO_ROWS || f->layout > FB_LAYOUT_PLANAR) {
		return "layout must be one of the FB_LAYOUT_* constants";
	}
	if (f->bit_order != FB_BIT_ORDER_MSB && f->bit_order != FB_BIT_ORDER_LSB) {
		return "bitOrder must be FB_BIT_ORDER_MSB or FB_BIT_ORDER_LSB";
	}
	if (f->byte_order != FB_BYTE_ORDER_MSB && f->byte_order != FB_BYTE_ORDER_LSB) {
		return "byteOrder must be FB_BYTE_ORDER_MSB or FB_BYTE_ORDER_LSB";
	}
	if (f->scan != FB_SCAN_TOP_DOWN && f->scan != FB_SCAN_BOTTOM_UP) {
		return "scan must be FB_SCAN_TOP_DOWN or FB_SCAN_BOTTOM_UP";
	}
	if (f->ink_count < 0 || f->ink_count > FB_MAX_INKS) {
		return "palette holds at most 16 inks";
	}

	f->bits = 0;
	f->levels = 0;

	switch (f->layout) {
		case FB_LAYOUT_MONO_ROWS:
		case FB_LAYOUT_MONO_PAGES:
			f->mode = FB_MODE_MONO;
			break;
		case FB_LAYOUT_INDEX2:
		case FB_LAYOUT_INDEX4:
		case FB_LAYOUT_INDEX8:
			f->bits = f->layout == FB_LAYOUT_INDEX2 ? 2 : f->layout == FB_LAYOUT_INDEX4 ? 4 : 8;
			f->levels = (1 << f->bits) - 1;
			f->mode = f->ink_count > 0 ? FB_MODE_INDEX : FB_MODE_GREY;
			break;
		case FB_LAYOUT_PLANAR:
			if (f->ink_count < 1) {
				return "a planar layout needs a palette";
			}
			f->mode = FB_MODE_PLANAR;
			break;
		default:
			f->mode = FB_MODE_RGB;
	}

	if (f->layout == FB_LAYOUT_RGBA8888) {
		if (f->channel_order < FB_CHANNELS_RGBA || f->channel_order > FB_CHANNELS_ABGR) {
			return "channelOrder of a 32-bit layout must be FB_CHANNELS_RGBA, _BGRA, _ARGB or _ABGR";
		}
	} else if (f->mode == FB_MODE_RGB) {
		if (f->channel_order != FB_CHANNELS_RGB && f->channel_order != FB_CHANNELS_BGR) {
			return "channelOrder of a 12 to 24-bit layout must be FB_CHANNELS_RGB or FB_CHANNELS_BGR";
		}
	} else if (f->channel_order != FB_CHANNELS_RGB) {
		return "channelOrder applies to the RGB layouts only";
	}

	if (f->ink_count > 0 && f->mode != FB_MODE_INDEX && f->mode != FB_MODE_PLANAR) {
		return "palette applies to the index and planar layouts only";
	}
	for (int k = 0; k < f->ink_count; k++) {
		if (f->inks[k].rgb > 0xFFFFFF) {
			return "a palette colour must be 0xRRGGBB";
		}
		if (f->mode == FB_MODE_INDEX && f->inks[k].code > (uint32_t) f->levels) {
			return "a palette code must fit the layout's bits";
		}
	}

	return NULL;
}

int fb_format_same_storage(const fb_format *a, const fb_format *b)
{
	if (a->layout != b->layout || a->bit_order != b->bit_order || a->byte_order != b->byte_order
			|| a->channel_order != b->channel_order || a->ink_count != b->ink_count) {
		return 0;
	}
	for (int k = 0; k < a->ink_count; k++) {
		if (a->inks[k].rgb != b->inks[k].rgb || a->inks[k].code != b->inks[k].code || a->inks[k].inverted != b->inks[k].inverted) {
			return 0;
		}
	}

	return 1;
}

static size_t fb_row_bytes(const fb_format *f, int width)
{
	switch (f->layout) {
		case FB_LAYOUT_MONO_ROWS:
		case FB_LAYOUT_PLANAR:
			return ((size_t) width + 7) / 8;
		case FB_LAYOUT_INDEX2:
		case FB_LAYOUT_INDEX4:
		case FB_LAYOUT_INDEX8:
			return ((size_t) width * f->bits + 7) / 8;
		case FB_LAYOUT_RGB444:
			return (((size_t) width + 1) / 2) * 3;
		case FB_LAYOUT_RGB565:
			return (size_t) width * 2;
		case FB_LAYOUT_RGB666:
		case FB_LAYOUT_RGB888:
			return (size_t) width * 3;
		case FB_LAYOUT_RGBA8888:
			return (size_t) width * 4;
		default:
			return 0;
	}
}

/* Bytes a pixel takes in the layouts that give every pixel whole bytes of its own; 0 for the bit-packed ones. */
static size_t fb_pixel_bytes(const fb_format *f)
{
	switch (f->layout) {
		case FB_LAYOUT_INDEX8:
			return 1;
		case FB_LAYOUT_RGB565:
			return 2;
		case FB_LAYOUT_RGB666:
		case FB_LAYOUT_RGB888:
			return 3;
		case FB_LAYOUT_RGBA8888:
			return 4;
		default:
			return 0;
	}
}

size_t fb_plane_size(int width, int height)
{
	return (((size_t) width + 7) / 8) * (size_t) height;
}

size_t fb_size(const fb_format *f, int width, int height)
{
	if (f->layout == FB_LAYOUT_MONO_PAGES) {
		return (size_t) width * (((size_t) height + 7) / 8);
	}
	if (f->layout == FB_LAYOUT_PLANAR) {
		return fb_plane_size(width, height) * (size_t) f->ink_count;
	}

	return fb_row_bytes(f, width) * (size_t) height;
}

void fb_granularity(const fb_format *f, int width, int *unit_width, int *unit_height)
{
	*unit_width = f->layout == FB_LAYOUT_MONO_PAGES ? width : 1;
	*unit_height = f->layout == FB_LAYOUT_MONO_PAGES ? 8 : 1;
}

/* A rect whose rows are each one run of whole bytes: row r of plane k starts at k * plane + offset + r * stride. */
typedef struct {
	size_t offset;
	size_t length;
	size_t stride;
	size_t plane;
	int planes;
} fb_runs;

/*
 * True, with the runs, when a rect's rows can be moved as bytes: any rect of a
 * layout with whole-byte pixels, and whole rows of any layout that packs row
 * by row. A whole row takes its padding bits along; nothing ever writes them,
 * so every row of every buffer of a format carries the same ones.
 */
static int fb_runs_of(const fb_format *f, int width, int height, fb_rect rect, fb_runs *runs)
{
	size_t pixel = fb_pixel_bytes(f);

	if (pixel > 0) {
		*runs = (fb_runs) {((size_t) rect.y * (size_t) width + (size_t) rect.x) * pixel, (size_t) rect.width * pixel, (size_t) width * pixel, 0, 1};
		return 1;
	}
	if (f->layout != FB_LAYOUT_MONO_PAGES && rect.x == 0 && rect.width == width) {
		size_t row = fb_row_bytes(f, width);

		*runs = (fb_runs) {(size_t) rect.y * row, row, row, fb_plane_size(width, height), f->layout == FB_LAYOUT_PLANAR ? f->ink_count : 1};
		return 1;
	}

	return 0;
}

/* RGB and BGR at 12 bits differ by swapping the outer nibbles; at 16, the two five-bit fields. Each swap is its own inverse. */
static uint32_t fb_swap444(uint32_t word)
{
	return ((word & 0xF) << 8) | (word & 0xF0) | (word >> 8);
}

static uint32_t fb_swap565(uint32_t word)
{
	return ((word & 0x1F) << 11) | (word & 0x07E0) | (word >> 11);
}

static void fb_put_bit(uint8_t *byte, int bit, uint32_t on)
{
	*byte = on ? (uint8_t) (*byte | (1u << bit)) : (uint8_t) (*byte & ~(1u << bit));
}

/* The word of the pixel at `p`, for the layouts fb_pixel_bytes() counts. */
static inline uint32_t fb_load(const fb_format *f, const uint8_t *p)
{
	int bgr = f->channel_order == FB_CHANNELS_BGR;

	switch (f->layout) {
		case FB_LAYOUT_INDEX8:
			return p[0];
		case FB_LAYOUT_RGB565: {
			uint32_t stored = f->byte_order == FB_BYTE_ORDER_MSB
				? ((uint32_t) p[0] << 8) | (uint32_t) p[1]
				: ((uint32_t) p[1] << 8) | (uint32_t) p[0];

			return bgr ? fb_swap565(stored) : stored;
		}
		case FB_LAYOUT_RGB666:
		case FB_LAYOUT_RGB888: {
			uint32_t mask = f->layout == FB_LAYOUT_RGB666 ? 0xFC : 0xFF;

			return (((uint32_t) p[bgr ? 2 : 0] & mask) << 16) | (((uint32_t) p[1] & mask) << 8) | ((uint32_t) p[bgr ? 0 : 2] & mask);
		}
		default: {
			const uint8_t *at = fb_rgba_at[f->channel_order];

			return ((uint32_t) p[at[0]] << 24) | ((uint32_t) p[at[1]] << 16) | ((uint32_t) p[at[2]] << 8) | (uint32_t) p[at[3]];
		}
	}
}

/* Store a word in the pixel at `p`, for the layouts fb_pixel_bytes() counts. */
static inline void fb_store(const fb_format *f, uint8_t *p, uint32_t word)
{
	int bgr = f->channel_order == FB_CHANNELS_BGR;

	switch (f->layout) {
		case FB_LAYOUT_INDEX8:
			p[0] = (uint8_t) word;
			return;
		case FB_LAYOUT_RGB565: {
			uint32_t stored = bgr ? fb_swap565(word & 0xFFFF) : word & 0xFFFF;

			p[f->byte_order == FB_BYTE_ORDER_MSB ? 0 : 1] = (uint8_t) (stored >> 8);
			p[f->byte_order == FB_BYTE_ORDER_MSB ? 1 : 0] = (uint8_t) (stored & 0xFF);
			return;
		}
		case FB_LAYOUT_RGB666:
		case FB_LAYOUT_RGB888: {
			uint32_t mask = f->layout == FB_LAYOUT_RGB666 ? 0xFC : 0xFF;

			p[bgr ? 2 : 0] = (uint8_t) ((word >> 16) & mask);
			p[1] = (uint8_t) ((word >> 8) & mask);
			p[bgr ? 0 : 2] = (uint8_t) (word & mask);
			return;
		}
		default: {
			const uint8_t *at = fb_rgba_at[f->channel_order];

			p[at[0]] = (uint8_t) (word >> 24);
			p[at[1]] = (uint8_t) (word >> 16);
			p[at[2]] = (uint8_t) (word >> 8);
			p[at[3]] = (uint8_t) word;
			return;
		}
	}
}

uint32_t fb_get(const fb_format *f, const uint8_t *bytes, int width, int height, int x, int y)
{
	int bgr = f->channel_order == FB_CHANNELS_BGR;
	size_t pixel = fb_pixel_bytes(f);

	if (pixel > 0) {
		return fb_load(f, bytes + ((size_t) y * (size_t) width + (size_t) x) * pixel);
	}

	switch (f->layout) {
		case FB_LAYOUT_MONO_ROWS:
			return (bytes[(size_t) y * fb_row_bytes(f, width) + (size_t) (x >> 3)]
				>> (f->bit_order == FB_BIT_ORDER_MSB ? 7 - (x & 7) : x & 7)) & 1u;
		case FB_LAYOUT_MONO_PAGES:
			return (bytes[(size_t) (y >> 3) * (size_t) width + (size_t) x]
				>> (f->bit_order == FB_BIT_ORDER_LSB ? y & 7 : 7 - (y & 7))) & 1u;
		case FB_LAYOUT_INDEX2:
		case FB_LAYOUT_INDEX4: {
			int per_byte = 8 / f->bits;
			int slot = x % per_byte;
			int shift = (f->bit_order == FB_BIT_ORDER_MSB ? per_byte - 1 - slot : slot) * f->bits;

			return ((uint32_t) bytes[(size_t) y * fb_row_bytes(f, width) + (size_t) (x / per_byte)] >> shift) & (uint32_t) f->levels;
		}
		case FB_LAYOUT_RGB444: {
			const uint8_t *p = bytes + (size_t) y * fb_row_bytes(f, width) + (size_t) (x / 2) * 3;
			uint32_t stored = (x & 1) == 0
				? ((uint32_t) p[0] << 4) | ((uint32_t) p[1] >> 4)
				: (((uint32_t) p[1] & 0x0F) << 8) | (uint32_t) p[2];

			return bgr ? fb_swap444(stored) : stored;
		}
		case FB_LAYOUT_PLANAR: {
			size_t plane = fb_plane_size(width, height);
			size_t at = (size_t) y * fb_row_bytes(f, width) + (size_t) (x >> 3);
			int bit = f->bit_order == FB_BIT_ORDER_MSB ? 7 - (x & 7) : x & 7;
			uint32_t word = 0;

			for (int k = 0; k < f->ink_count; k++) {
				uint32_t stored = ((uint32_t) bytes[plane * (size_t) k + at] >> bit) & 1u;
				if ((stored ^ f->inks[k].inverted) == 1u) {
					word |= 1u << k;
				}
			}

			return word;
		}
		default:
			return 0;
	}
}

void fb_set(const fb_format *f, uint8_t *bytes, int width, int height, int x, int y, uint32_t word)
{
	int bgr = f->channel_order == FB_CHANNELS_BGR;
	size_t pixel = fb_pixel_bytes(f);

	if (pixel > 0) {
		fb_store(f, bytes + ((size_t) y * (size_t) width + (size_t) x) * pixel, word);
		return;
	}

	switch (f->layout) {
		case FB_LAYOUT_MONO_ROWS:
			fb_put_bit(&bytes[(size_t) y * fb_row_bytes(f, width) + (size_t) (x >> 3)],
				f->bit_order == FB_BIT_ORDER_MSB ? 7 - (x & 7) : x & 7, word & 1u);
			return;
		case FB_LAYOUT_MONO_PAGES:
			fb_put_bit(&bytes[(size_t) (y >> 3) * (size_t) width + (size_t) x],
				f->bit_order == FB_BIT_ORDER_LSB ? y & 7 : 7 - (y & 7), word & 1u);
			return;
		case FB_LAYOUT_INDEX2:
		case FB_LAYOUT_INDEX4: {
			int per_byte = 8 / f->bits;
			int slot = x % per_byte;
			int shift = (f->bit_order == FB_BIT_ORDER_MSB ? per_byte - 1 - slot : slot) * f->bits;
			uint8_t *p = &bytes[(size_t) y * fb_row_bytes(f, width) + (size_t) (x / per_byte)];

			*p = (uint8_t) ((*p & ~((uint32_t) f->levels << shift)) | ((word & (uint32_t) f->levels) << shift));
			return;
		}
		case FB_LAYOUT_RGB444: {
			uint8_t *p = bytes + (size_t) y * fb_row_bytes(f, width) + (size_t) (x / 2) * 3;
			uint32_t stored = bgr ? fb_swap444(word & 0xFFF) : word & 0xFFF;

			if ((x & 1) == 0) {
				p[0] = (uint8_t) (stored >> 4);
				p[1] = (uint8_t) ((p[1] & 0x0F) | ((stored & 0xF) << 4));
			} else {
				p[1] = (uint8_t) ((p[1] & 0xF0) | (stored >> 8));
				p[2] = (uint8_t) (stored & 0xFF);
			}
			return;
		}
		case FB_LAYOUT_PLANAR: {
			size_t plane = fb_plane_size(width, height);
			size_t at = (size_t) y * fb_row_bytes(f, width) + (size_t) (x >> 3);
			int bit = f->bit_order == FB_BIT_ORDER_MSB ? 7 - (x & 7) : x & 7;

			for (int k = 0; k < f->ink_count; k++) {
				fb_put_bit(&bytes[plane * (size_t) k + at], bit, ((word >> k) & 1u) ^ f->inks[k].inverted);
			}
			return;
		}
	}
}

void fb_fill_rect(const fb_format *f, uint8_t *bytes, int width, int height, fb_rect rect, uint32_t word)
{
	fb_runs runs;

	if (f->layout == FB_LAYOUT_MONO_PAGES && (rect.y & 7) == 0 && (rect.height & 7) == 0) {
		/* Whole pages: every byte under the rect is all lit or all dark. */
		for (int y = rect.y; y < rect.y + rect.height; y += 8) {
			memset(bytes + (size_t) (y >> 3) * (size_t) width + (size_t) rect.x, (word & 1u) ? 0xFF : 0x00, (size_t) rect.width);
		}
		return;
	}

	/* Pixel by pixel: the first row when the others can be copied from it, every row when they cannot. */
	int by_runs = fb_runs_of(f, width, height, rect, &runs);
	int set_rows = by_runs ? 1 : rect.height;

	for (int y = rect.y; y < rect.y + set_rows; y++) {
		for (int x = rect.x; x < rect.x + rect.width; x++) {
			fb_set(f, bytes, width, height, x, y, word);
		}
	}
	if (!by_runs) {
		return;
	}
	for (int k = 0; k < runs.planes; k++) {
		uint8_t *first = bytes + runs.plane * (size_t) k + runs.offset;

		for (int row = 1; row < rect.height; row++) {
			memcpy(first + runs.stride * (size_t) row, first, runs.length);
		}
	}
}

void fb_blank(const fb_format *f, uint8_t *bytes, int width, int height)
{
	memset(bytes, 0, fb_size(f, width, height));
	if (f->layout == FB_LAYOUT_PLANAR) {
		fb_fill_rect(f, bytes, width, height, (fb_rect) {0, 0, width, height}, 0);
	}
}

static uint32_t fb_luma(int red, int green, int blue)
{
	return 2126u * (uint32_t) red + 7152u * (uint32_t) green + 722u * (uint32_t) blue;
}

/* round(value / max * 255) in integers. */
static uint32_t fb_widen(uint32_t value, uint32_t max)
{
	return (value * 510u + max) / (max * 2u);
}

static uint32_t fb_opaque(uint32_t red, uint32_t green, uint32_t blue)
{
	return (red << 24) | (green << 16) | (blue << 8) | 0xFFu;
}

static uint32_t fb_distance(int red, int green, int blue, uint32_t rgb)
{
	int dr = red - (int) ((rgb >> 16) & 0xFF);
	int dg = green - (int) ((rgb >> 8) & 0xFF);
	int db = blue - (int) (rgb & 0xFF);

	return (uint32_t) (dr * dr + dg * dg + db * db);
}

uint32_t fb_map(const fb_format *f, int red, int green, int blue, int alpha)
{
	switch (f->mode) {
		case FB_MODE_MONO:
			return fb_luma(red, green, blue) * 2u >= FB_LUMA_MAX ? 1u : 0u;
		case FB_MODE_GREY:
			return (uint32_t) (((uint64_t) fb_luma(red, green, blue) * (uint64_t) f->levels * 2u + FB_LUMA_MAX) / ((uint64_t) FB_LUMA_MAX * 2u));
		case FB_MODE_INDEX:
		case FB_MODE_PLANAR: {
			/* The nearest palette entry in RGB; the earlier entry wins a tie. Planar starts from paper. */
			uint32_t best = 0;
			uint32_t best_distance = f->mode == FB_MODE_PLANAR ? fb_distance(red, green, blue, 0xFFFFFF) : UINT32_MAX;

			for (int k = 0; k < f->ink_count; k++) {
				uint32_t distance = fb_distance(red, green, blue, f->inks[k].rgb);
				if (distance < best_distance) {
					best_distance = distance;
					best = f->mode == FB_MODE_PLANAR ? 1u << k : f->inks[k].code;
				}
			}

			return best;
		}
		default:
			break;
	}

	uint32_t r = (uint32_t) red, g = (uint32_t) green, b = (uint32_t) blue;

	switch (f->layout) {
		case FB_LAYOUT_RGB444:
			return ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
		case FB_LAYOUT_RGB565:
			return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
		case FB_LAYOUT_RGB666:
			return ((r & 0xFC) << 16) | ((g & 0xFC) << 8) | (b & 0xFC);
		case FB_LAYOUT_RGB888:
			return (r << 16) | (g << 8) | b;
		default:
			return (r << 24) | (g << 16) | (b << 8) | (uint32_t) alpha;
	}
}

uint32_t fb_unmap(const fb_format *f, uint32_t word)
{
	switch (f->mode) {
		case FB_MODE_MONO:
			return word ? 0xFFFFFFFFu : 0x000000FFu;
		case FB_MODE_GREY: {
			uint32_t grey = fb_widen(word & (uint32_t) f->levels, (uint32_t) f->levels);

			return fb_opaque(grey, grey, grey);
		}
		case FB_MODE_INDEX:
			for (int k = 0; k < f->ink_count; k++) {
				if (f->inks[k].code == word) {
					return (f->inks[k].rgb << 8) | 0xFFu;
				}
			}
			return 0xFFFFFFFFu;
		case FB_MODE_PLANAR:
			/* The lowest set bit's ink; no ink, or a bit past the palette, reads as paper. */
			for (int k = 0; k < f->ink_count; k++) {
				if ((word >> k) & 1u) {
					return (f->inks[k].rgb << 8) | 0xFFu;
				}
			}
			return 0xFFFFFFFFu;
		default:
			break;
	}

	switch (f->layout) {
		case FB_LAYOUT_RGB444:
			return fb_opaque(((word >> 8) & 0xF) * 17u, ((word >> 4) & 0xF) * 17u, (word & 0xF) * 17u);
		case FB_LAYOUT_RGB565:
			return fb_opaque(fb_widen((word >> 11) & 0x1F, 31), fb_widen((word >> 5) & 0x3F, 63), fb_widen(word & 0x1F, 31));
		case FB_LAYOUT_RGB666:
			return fb_opaque(fb_widen((word >> 18) & 0x3F, 63), fb_widen((word >> 10) & 0x3F, 63), fb_widen((word >> 2) & 0x3F, 63));
		case FB_LAYOUT_RGB888:
			return ((word & 0xFFFFFFu) << 8) | 0xFFu;
		default:
			return word;
	}
}

/* 0xRRGGBBAA to a format's word. */
static inline uint32_t fb_map_rgba(const fb_format *f, uint32_t rgba)
{
	return fb_map(f, (int) (rgba >> 24), (int) ((rgba >> 16) & 0xFF), (int) ((rgba >> 8) & 0xFF), (int) (rgba & 0xFF));
}

/* 32-bit pixels from one channel order to another: the same four bytes, moved. */
static void fb_reorder(uint8_t *restrict out, const uint8_t *restrict in, size_t pixels, int from, int to)
{
	uint8_t at[4];

	if (from == to) {
		memcpy(out, in, pixels * 4);
		return;
	}
	for (int channel = 0; channel < 4; channel++) {
		at[fb_rgba_at[to][channel]] = fb_rgba_at[from][channel];
	}
	for (size_t i = 0; i < pixels; i++, in += 4, out += 4) {
		out[0] = in[at[0]];
		out[1] = in[at[1]];
		out[2] = in[at[2]];
		out[3] = in[at[3]];
	}
}

void fb_region(const fb_format *f, const uint8_t *bytes, int width, int height, fb_rect rect, const fb_format *to, uint8_t *out)
{
	int same = to == f;
	size_t from_pixel = fb_pixel_bytes(f);
	size_t to_pixel = fb_pixel_bytes(to);
	fb_runs runs;

	if (same && fb_runs_of(f, width, height, rect, &runs)) {
		/* As stored, a run at a time: a row of the rect is `runs.length` bytes in `out` too. */
		size_t out_plane = fb_plane_size(rect.width, rect.height);

		for (int k = 0; k < runs.planes; k++) {
			for (int y = 0; y < rect.height; y++) {
				int row = to->scan == FB_SCAN_BOTTOM_UP ? rect.height - 1 - y : y;

				memcpy(out + out_plane * (size_t) k + runs.length * (size_t) row,
					bytes + runs.plane * (size_t) k + runs.offset + runs.stride * (size_t) y, runs.length);
			}
		}
		return;
	}

	if (!same && from_pixel > 0 && to_pixel > 0) {
		/* Whole-byte pixels on both sides: walk the two rows, with no bits of `out` left to blank. */
		int reorder = f->layout == FB_LAYOUT_RGBA8888 && to->layout == FB_LAYOUT_RGBA8888;

		for (int y = 0; y < rect.height; y++) {
			int row = to->scan == FB_SCAN_BOTTOM_UP ? rect.height - 1 - y : y;
			const uint8_t *p = bytes + ((size_t) (rect.y + y) * (size_t) width + (size_t) rect.x) * from_pixel;
			uint8_t *q = out + (size_t) row * (size_t) rect.width * to_pixel;

			if (reorder) {
				fb_reorder(q, p, (size_t) rect.width, f->channel_order, to->channel_order);
				continue;
			}
			for (int x = 0; x < rect.width; x++, p += from_pixel, q += to_pixel) {
				fb_store(to, q, fb_map_rgba(to, fb_unmap(f, fb_load(f, p))));
			}
		}
		return;
	}

	fb_blank(to, out, rect.width, rect.height);
	for (int y = 0; y < rect.height; y++) {
		int row = to->scan == FB_SCAN_BOTTOM_UP ? rect.height - 1 - y : y;
		for (int x = 0; x < rect.width; x++) {
			uint32_t word = fb_get(f, bytes, width, height, rect.x + x, rect.y + y);

			fb_set(to, out, rect.width, rect.height, x, row, same ? word : fb_map_rgba(to, fb_unmap(f, word)));
		}
	}
}

static inline uint8_t *fb_put_rgba8(uint8_t *out, uint32_t rgba)
{
	out[0] = (uint8_t) (rgba >> 24);
	out[1] = (uint8_t) (rgba >> 16);
	out[2] = (uint8_t) (rgba >> 8);
	out[3] = (uint8_t) rgba;

	return out + 4;
}

void fb_rgba8(const fb_format *f, const uint8_t *bytes, int width, int height, uint8_t *out)
{
	size_t pixel = fb_pixel_bytes(f);
	size_t pixels = (size_t) width * (size_t) height;

	if (f->layout == FB_LAYOUT_RGBA8888) {
		fb_reorder(out, bytes, pixels, f->channel_order, FB_CHANNELS_RGBA);
		return;
	}
	if (pixel > 0) {
		for (size_t i = 0; i < pixels; i++, bytes += pixel) {
			out = fb_put_rgba8(out, fb_unmap(f, fb_load(f, bytes)));
		}
		return;
	}
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			out = fb_put_rgba8(out, fb_unmap(f, fb_get(f, bytes, width, height, x, y)));
		}
	}
}

void fb_blit_rgba8(const fb_format *f, uint8_t *bytes, int width, int height, const uint8_t *rgba8, int source_width, int offset_x, int offset_y, fb_rect target)
{
	size_t pixel = fb_pixel_bytes(f);

	for (int y = target.y; y < target.y + target.height; y++) {
		const uint8_t *p = rgba8 + ((size_t) (y - offset_y) * (size_t) source_width + (size_t) (target.x - offset_x)) * 4;
		uint8_t *q = bytes + ((size_t) y * (size_t) width + (size_t) target.x) * pixel;

		if (f->layout == FB_LAYOUT_RGBA8888) {
			fb_reorder(q, p, (size_t) target.width, FB_CHANNELS_RGBA, f->channel_order);
			continue;
		}
		if (pixel > 0) {
			for (int x = 0; x < target.width; x++, p += 4, q += pixel) {
				fb_store(f, q, fb_map(f, p[0], p[1], p[2], p[3]));
			}
			continue;
		}
		for (int x = target.x; x < target.x + target.width; x++, p += 4) {
			fb_set(f, bytes, width, height, x, y, fb_map(f, p[0], p[1], p[2], p[3]));
		}
	}
}

void fb_copy(const fb_format *f, uint8_t *bytes, const uint8_t *source, int width, int height, fb_rect rect)
{
	fb_runs runs;

	if (rect.x == 0 && rect.y == 0 && rect.width == width && rect.height == height) {
		memmove(bytes, source, fb_size(f, width, height));
		return;
	}
	if (fb_runs_of(f, width, height, rect, &runs)) {
		for (int k = 0; k < runs.planes; k++) {
			for (int row = 0; row < rect.height; row++) {
				size_t at = runs.plane * (size_t) k + runs.offset + runs.stride * (size_t) row;

				memmove(bytes + at, source + at, runs.length);
			}
		}
		return;
	}
	for (int y = rect.y; y < rect.y + rect.height; y++) {
		for (int x = rect.x; x < rect.x + rect.width; x++) {
			fb_set(f, bytes, width, height, x, y, fb_get(f, source, width, height, x, y));
		}
	}
}

/* One pixel, source-over at alpha `a` where the format blends; the colour itself from 128 up where it cannot. */
static inline void fb_blend(const fb_format *f, uint8_t *bytes, int width, int height, int x, int y, uint32_t red, uint32_t green, uint32_t blue, uint32_t a, int blends)
{
	if (a == 0 || (!blends && a < 128)) {
		return;
	}
	if (a == 255 || !blends) {
		fb_set(f, bytes, width, height, x, y, fb_map(f, (int) red, (int) green, (int) blue, 255));
		return;
	}

	uint32_t d = fb_unmap(f, fb_get(f, bytes, width, height, x, y));

	fb_set(f, bytes, width, height, x, y, fb_map(f,
		(int) ((red * a + (d >> 24) * (255 - a) + 127) / 255),
		(int) ((green * a + ((d >> 16) & 0xFF) * (255 - a) + 127) / 255),
		(int) ((blue * a + ((d >> 8) & 0xFF) * (255 - a) + 127) / 255),
		(int) ((255 * a + (d & 0xFF) * (255 - a) + 127) / 255)));
}

void fb_paint_spans(const fb_format *f, uint8_t *bytes, int width, int height, const uint8_t *spans, size_t count, uint32_t rgba)
{
	uint32_t red = rgba >> 24, green = (rgba >> 16) & 0xFF, blue = (rgba >> 8) & 0xFF, alpha = rgba & 0xFF;
	uint32_t word = fb_map(f, (int) red, (int) green, (int) blue, (int) alpha);
	int blends = f->mode == FB_MODE_RGB || f->mode == FB_MODE_GREY;

	for (size_t i = 0; i < count; i++) {
		const uint8_t *p = spans + i * FB_SPAN_BYTES;
		int y = p[0] | (p[1] << 8), x = p[2] | (p[3] << 8), length = p[4] | (p[5] << 8);
		uint32_t a = (alpha * p[6] + 127) / 255;

		if (a == 0 || (!blends && a < 128)) {
			continue;
		}
		if (a == 255 || !blends) {
			fb_fill_rect(f, bytes, width, height, (fb_rect) {x, y, length, 1}, word);
			continue;
		}
		for (int px = x; px < x + length; px++) {
			fb_blend(f, bytes, width, height, px, y, red, green, blue, a, 1);
		}
	}
}

static inline int fb_clamp_int(int value, int low, int high)
{
	return value < low ? low : (value > high ? high : value);
}

void fb_paint_rgba8(const fb_format *f, uint8_t *bytes, int width, int height, const uint8_t *rgba8, int source_width, int source_height,
	const double inverse[6], fb_rect target, int opacity, int smooth, int row)
{
	double ia = inverse[0], ib = inverse[1], ic = inverse[2], id = inverse[3], ie = inverse[4], jf = inverse[5];
	int blends = f->mode == FB_MODE_RGB || f->mode == FB_MODE_GREY;

	for (int y = target.y; y < target.y + target.height; y++) {
		double py = (double) (y + row) + 0.5;

		for (int x = target.x; x < target.x + target.width; x++) {
			double px = (double) x + 0.5;
			double u = ia * px + ic * py + ie;
			double v = ib * px + id * py + jf;
			uint32_t red, green, blue, alpha;

			if (!(u >= 0 && u < (double) source_width && v >= 0 && v < (double) source_height)) {
				continue;
			}

			if (!smooth) {
				const uint8_t *p = rgba8 + ((size_t) floor(v) * (size_t) source_width + (size_t) floor(u)) * 4;

				red = p[0];
				green = p[1];
				blue = p[2];
				alpha = p[3];
			} else {
				/* The four pixels around the point, weights in 1/256ths, colours weighed by their alpha. */
				double fx = u - 0.5, fy = v - 0.5;
				double x0 = floor(fx), y0 = floor(fy);
				uint32_t tx = (uint32_t) floor((fx - x0) * 256), ty = (uint32_t) floor((fy - y0) * 256);
				int xa = fb_clamp_int((int) x0, 0, source_width - 1), xb = fb_clamp_int((int) x0 + 1, 0, source_width - 1);
				int ya = fb_clamp_int((int) y0, 0, source_height - 1), yb = fb_clamp_int((int) y0 + 1, 0, source_height - 1);
				const int at[4][2] = {{ya, xa}, {ya, xb}, {yb, xa}, {yb, xb}};
				const uint32_t weights[4] = {(256 - tx) * (256 - ty), tx * (256 - ty), (256 - tx) * ty, tx * ty};
				uint64_t sum = 0, reds = 0, greens = 0, blues = 0;

				for (int k = 0; k < 4; k++) {
					const uint8_t *p = rgba8 + ((size_t) at[k][0] * (size_t) source_width + (size_t) at[k][1]) * 4;
					uint64_t weighed = (uint64_t) weights[k] * p[3];

					sum += weighed;
					reds += weighed * p[0];
					greens += weighed * p[1];
					blues += weighed * p[2];
				}
				if (sum == 0) {
					continue;
				}
				red = (uint32_t) ((reds + sum / 2) / sum);
				green = (uint32_t) ((greens + sum / 2) / sum);
				blue = (uint32_t) ((blues + sum / 2) / sum);
				alpha = (uint32_t) ((sum + 32768) >> 16);
			}

			fb_blend(f, bytes, width, height, x, y, red, green, blue, (alpha * (uint32_t) opacity + 127) / 255, blends);
		}
	}
}

void fb_plane(const fb_format *f, const uint8_t *bytes, int width, int height, uint32_t word, uint8_t *out)
{
	size_t row_bytes = ((size_t) width + 7) / 8;

	memset(out, 0, fb_plane_size(width, height));
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (fb_get(f, bytes, width, height, x, y) == word) {
				out[(size_t) y * row_bytes + (size_t) (x >> 3)] |= (uint8_t) (0x80u >> (x & 7));
			}
		}
	}
}
