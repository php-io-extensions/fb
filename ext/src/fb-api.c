#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "php.h"
#include "php_ext.h"
#include "fb-api.h"
#include <stdint.h>
#include <string.h>
#include <math.h>

#define H(z) ((fb_buffer *)(uintptr_t)(zend_long) Z_LVAL_P(z))
#define L(z) ((int) Z_LVAL_P(z))

/* ---- format resolution ------------------------------------------------- */

static int fb_resolve(fb_format *f, int pixel_format, int bit_depth, int bit_order, int endianness, int page_axis, int scan, zval *palette)
{
    memset(f, 0, sizeof *f);
    f->pixel_format = pixel_format; f->bit_depth = bit_depth; f->scan = scan;
    f->bit_order = bit_order; f->endianness = endianness; f->page_axis = page_axis;

    if (palette && Z_TYPE_P(palette) == IS_ARRAY) {
        zval *entry;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(palette), entry) {
            if (f->palette.n >= FB_MAX_PALETTE || Z_TYPE_P(entry) != IS_ARRAY) return 0;
            zval *rgb = zend_hash_index_find(Z_ARRVAL_P(entry), 0);
            zval *inv = zend_hash_index_find(Z_ARRVAL_P(entry), 1);
            zval *code = zend_hash_index_find(Z_ARRVAL_P(entry), 2);
            f->palette.rgb[f->palette.n] = rgb ? (uint32_t) Z_LVAL_P(rgb) : 0;
            f->palette.inverted[f->palette.n] = inv ? (int) Z_LVAL_P(inv) : 0;
            f->palette.code[f->palette.n] = code ? (int) Z_LVAL_P(code) : f->palette.n;
            f->palette.n++;
        } ZEND_HASH_FOREACH_END();
    }

    int has_palette = f->palette.n > 0;
    if (pixel_format == 1 && bit_depth == 1) {            /* mono_horizontal */
        f->layout = FB_L_MONO_H; f->mode = FB_M_MONO;
        if (f->bit_order < 0) f->bit_order = 0;
    } else if (pixel_format == 0 && bit_depth == 1) {     /* mono_vertical_page */
        if (f->bit_order < 0) f->bit_order = 1;
        if (f->page_axis < 0) f->page_axis = 1;
        f->layout = f->page_axis == 0 ? FB_L_MONO_H : FB_L_MONO_VP; f->mode = FB_M_MONO;
    } else if (pixel_format == 3 && bit_depth == 1 && has_palette) {   /* planar */
        f->layout = FB_L_PLANAR; f->mode = FB_M_PLANAR; f->planes = f->palette.n;
        if (f->bit_order < 0) f->bit_order = 0;
    } else if (pixel_format == 2) {                        /* row_major */
        switch (bit_depth) {
            case 2: case 4: case 8:
                f->layout = FB_L_INDEX; f->bits = bit_depth;
                f->mode = has_palette ? FB_M_INDEX : (bit_depth == 8 ? FB_M_GREY : FB_M_INDEX);
                if (f->bit_order < 0) f->bit_order = 0;
                break;
            case 12: f->layout = FB_L_RGB444;   f->mode = FB_M_RGB; break;
            case 16: f->layout = FB_L_RGB565;   f->mode = FB_M_RGB; if (f->endianness < 0) f->endianness = 1; break;
            case 18: f->layout = FB_L_RGB666;   f->mode = FB_M_RGB; break;
            case 24: f->layout = FB_L_RGB888;   f->mode = FB_M_RGB; break;
            case 32: f->layout = FB_L_RGBA8888; f->mode = FB_M_RGB; break;
            default: return 0;
        }
    } else {
        return 0;
    }
    if (f->scan < 0) f->scan = 0;
    return 1;
}

/* ---- DERIVE: layout math ---------------------------------------------- */

static int fb_rowbytes(const fb_format *f, int w)
{
    switch (f->layout) {
        case FB_L_MONO_H:
        case FB_L_PLANAR:
            return (w + 7) / 8;
        case FB_L_INDEX:
            return (w * f->bits + 7) / 8;
        case FB_L_RGB444:
            return ((w + 1) / 2) * 3;
        case FB_L_RGB565:
            return w * 2;
        case FB_L_RGB666:
        case FB_L_RGB888:
            return w * 3;
        case FB_L_RGBA8888:
            return w * 4;
        default:
            return 0;
    }
}

static size_t fb_bytes_for(const fb_format *f, int w, int h)
{
    if (f->layout == FB_L_MONO_VP) {
        return (size_t) w * ((h + 7) / 8);
    }
    if (f->layout == FB_L_PLANAR) {
        return (size_t) ((w + 7) / 8) * h * f->planes;
    }
    return (size_t) fb_rowbytes(f, w) * h;
}

static int fb_mono_bit(const fb_format *f, int coord)
{
    return f->bit_order == 0 ? (7 - (coord & 7)) : (coord & 7);
}

static void fb_put_bit(unsigned char *byte, int bit, int on)
{
    if (on) {
        *byte = (unsigned char) (*byte | (1u << bit));
    } else {
        *byte = (unsigned char) (*byte & ~(1u << bit));
    }
}

static zend_long fb_get_word(const fb_format *f, const unsigned char *bytes, int w, int h, int x, int y)
{
    (void) h;
    switch (f->layout) {
        case FB_L_MONO_H: {
            int rb = (w + 7) / 8;
            int bit = fb_mono_bit(f, x);
            return (bytes[y * rb + (x >> 3)] >> bit) & 1;
        }
        case FB_L_MONO_VP: {
            int bit = fb_mono_bit(f, y);
            return (bytes[(y >> 3) * w + x] >> bit) & 1;
        }
        case FB_L_INDEX: {
            int ppb = 8 / f->bits;
            int rb = (w * f->bits + 7) / 8;
            int slot = x % ppb;
            int shift = f->bit_order == 0 ? (ppb - 1 - slot) * f->bits : slot * f->bits;
            int mask = (1 << f->bits) - 1;
            return (bytes[y * rb + x / ppb] >> shift) & mask;
        }
        case FB_L_RGB444: {
            int rb = ((w + 1) / 2) * 3;
            const unsigned char *p = bytes + y * rb + (x / 2) * 3;
            if ((x & 1) == 0) {
                return ((p[0] >> 4) << 8) | ((p[0] & 0x0F) << 4) | (p[1] >> 4);
            }
            return ((p[1] & 0x0F) << 8) | (p[2] >> 4) << 4 | (p[2] & 0x0F);
        }
        case FB_L_RGB565: {
            const unsigned char *p = bytes + ((size_t) y * w + x) * 2;
            if (f->endianness == 0) {
                return (zend_long) p[0] | ((zend_long) p[1] << 8);
            }
            return ((zend_long) p[0] << 8) | (zend_long) p[1];
        }
        case FB_L_RGB666:
        case FB_L_RGB888: {
            const unsigned char *p = bytes + ((size_t) y * w + x) * 3;
            return ((zend_long) p[0] << 16) | ((zend_long) p[1] << 8) | (zend_long) p[2];
        }
        case FB_L_RGBA8888: {
            const unsigned char *p = bytes + ((size_t) y * w + x) * 4;
            return ((zend_long) p[0] << 24) | ((zend_long) p[1] << 16) | ((zend_long) p[2] << 8) | (zend_long) p[3];
        }
        case FB_L_PLANAR: {
            int rb = (w + 7) / 8;
            size_t plane = (size_t) rb * h;
            int bit = fb_mono_bit(f, x);
            zend_long word = 0;
            for (int k = 0; k < f->planes; k++) {
                int stored = (bytes[plane * (size_t) k + (size_t) y * rb + (x >> 3)] >> bit) & 1;
                int logical = stored ^ (f->palette.inverted[k] ? 1 : 0);
                if (logical) {
                    word |= (1L << k);
                }
            }
            return word;
        }
        default:
            return 0;
    }
}

static void fb_set_word(const fb_format *f, unsigned char *bytes, int w, int h, int x, int y, zend_long v)
{
    switch (f->layout) {
        case FB_L_MONO_H: {
            int rb = (w + 7) / 8;
            fb_put_bit(&bytes[y * rb + (x >> 3)], fb_mono_bit(f, x), (int) (v & 1));
            return;
        }
        case FB_L_MONO_VP: {
            fb_put_bit(&bytes[(y >> 3) * w + x], fb_mono_bit(f, y), (int) (v & 1));
            return;
        }
        case FB_L_INDEX: {
            int ppb = 8 / f->bits;
            int rb = (w * f->bits + 7) / 8;
            int slot = x % ppb;
            int shift = f->bit_order == 0 ? (ppb - 1 - slot) * f->bits : slot * f->bits;
            int mask = (1 << f->bits) - 1;
            unsigned char *p = &bytes[y * rb + x / ppb];
            *p = (unsigned char) ((*p & ~(mask << shift)) | (((int) v & mask) << shift));
            return;
        }
        case FB_L_RGB444: {
            int rb = ((w + 1) / 2) * 3;
            unsigned char *p = bytes + y * rb + (x / 2) * 3;
            int r = ((int) v >> 8) & 0xF, g = ((int) v >> 4) & 0xF, b = (int) v & 0xF;
            if ((x & 1) == 0) {
                p[0] = (unsigned char) ((r << 4) | g);
                p[1] = (unsigned char) ((p[1] & 0x0F) | (b << 4));
            } else {
                p[1] = (unsigned char) ((p[1] & 0xF0) | r);
                p[2] = (unsigned char) ((g << 4) | b);
            }
            return;
        }
        case FB_L_RGB565: {
            unsigned char *p = bytes + ((size_t) y * w + x) * 2;
            unsigned int word = (unsigned int) v & 0xFFFF;
            if (f->endianness == 0) {
                p[0] = (unsigned char) (word & 0xFF);
                p[1] = (unsigned char) (word >> 8);
            } else {
                p[0] = (unsigned char) (word >> 8);
                p[1] = (unsigned char) (word & 0xFF);
            }
            return;
        }
        case FB_L_RGB666: {
            unsigned char *p = bytes + ((size_t) y * w + x) * 3;
            p[0] = (unsigned char) (((int) v >> 16) & 0xFC);
            p[1] = (unsigned char) (((int) v >> 8) & 0xFC);
            p[2] = (unsigned char) ((int) v & 0xFC);
            return;
        }
        case FB_L_RGB888: {
            unsigned char *p = bytes + ((size_t) y * w + x) * 3;
            p[0] = (unsigned char) (((int) v >> 16) & 0xFF);
            p[1] = (unsigned char) (((int) v >> 8) & 0xFF);
            p[2] = (unsigned char) ((int) v & 0xFF);
            return;
        }
        case FB_L_RGBA8888: {
            unsigned char *p = bytes + ((size_t) y * w + x) * 4;
            p[0] = (unsigned char) (((unsigned int) v >> 24) & 0xFF);
            p[1] = (unsigned char) (((unsigned int) v >> 16) & 0xFF);
            p[2] = (unsigned char) (((unsigned int) v >> 8) & 0xFF);
            p[3] = (unsigned char) ((unsigned int) v & 0xFF);
            return;
        }
        case FB_L_PLANAR: {
            int rb = (w + 7) / 8;
            size_t plane = (size_t) rb * h;
            int bit = fb_mono_bit(f, x);
            for (int k = 0; k < f->planes; k++) {
                int logical = ((int) v >> k) & 1;
                int stored = logical ^ (f->palette.inverted[k] ? 1 : 0);
                fb_put_bit(&bytes[plane * (size_t) k + (size_t) y * rb + (x >> 3)], bit, stored);
            }
            return;
        }
        default:
            return;
    }
}

/* ---- DERIVE: mapper ----------------------------------------------------- */

static double fb_dist2(double r, double g, double b, double cr, double cg, double cb)
{
    double dr = r - cr, dg = g - cg, db = b - cb;
    return dr * dr + dg * dg + db * db;
}

static int fb_ch8(double c)
{
    return (int) round(c * 255.0);
}

static void fb_rgb_from_u32(uint32_t rgb, int *r, int *g, int *b)
{
    *r = (int) ((rgb >> 16) & 0xFF);
    *g = (int) ((rgb >> 8) & 0xFF);
    *b = (int) (rgb & 0xFF);
}

static zend_long fb_map(const fb_format *f, int r, int g, int b, int a)
{
    double rf = r / 255.0, gf = g / 255.0, bf = b / 255.0;
    double luma = 0.2126 * rf + 0.7152 * gf + 0.0722 * bf;

    if (f->mode == FB_M_MONO) {
        return luma >= 0.5 ? 1 : 0;
    }
    if (f->mode == FB_M_GREY) {
        return (zend_long) round(luma * 255.0);
    }
    if (f->mode == FB_M_PLANAR || f->mode == FB_M_INDEX) {
        zend_long best = 0;
        double best_d = 1e300;
        if (f->mode == FB_M_PLANAR) {
            best_d = fb_dist2(rf, gf, bf, 1.0, 1.0, 1.0);
            best = 0;
        }
        for (int k = 0; k < f->palette.n; k++) {
            int cr, cg, cb;
            fb_rgb_from_u32(f->palette.rgb[k], &cr, &cg, &cb);
            double d = fb_dist2(rf, gf, bf, cr / 255.0, cg / 255.0, cb / 255.0);
            if (d < best_d) {
                best_d = d;
                best = f->mode == FB_M_PLANAR ? (1L << k) : (zend_long) f->palette.code[k];
            }
        }
        return best;
    }

    switch (f->layout) {
        case FB_L_RGB444:
            return ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
        case FB_L_RGB565:
            return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
        case FB_L_RGB666:
            return ((r & 0xFC) << 16) | ((g & 0xFC) << 8) | (b & 0xFC);
        case FB_L_RGB888:
            return (r << 16) | (g << 8) | b;
        case FB_L_RGBA8888:
            return ((zend_long) (r & 255) << 24) | ((zend_long) (g & 255) << 16) | ((zend_long) (b & 255) << 8) | (zend_long) (a & 255);
        default:
            return 0;
    }
}

static void fb_unmap(const fb_format *f, zend_long v, int *r, int *g, int *b, int *a)
{
    *a = 255;
    if (f->mode == FB_M_MONO) {
        *r = *g = *b = v ? 255 : 0;
        return;
    }
    if (f->mode == FB_M_GREY) {
        int grey = (int) v & 255;
        *r = *g = *b = grey;
        return;
    }
    if (f->mode == FB_M_PLANAR) {
        if (v == 0) {
            *r = *g = *b = 255;
            return;
        }
        int k = 0;
        while (k < f->palette.n && (((int) v >> k) & 1) == 0) {
            k++;
        }
        if (k >= f->palette.n) {
            *r = *g = *b = 255;
            return;
        }
        fb_rgb_from_u32(f->palette.rgb[k], r, g, b);
        return;
    }
    if (f->mode == FB_M_INDEX) {
        for (int k = 0; k < f->palette.n; k++) {
            if (f->palette.code[k] == (int) v) {
                fb_rgb_from_u32(f->palette.rgb[k], r, g, b);
                return;
            }
        }
        *r = *g = *b = 255;
        return;
    }

    switch (f->layout) {
        case FB_L_RGB444:
            *r = fb_ch8(((v >> 8) & 0xF) / 15.0);
            *g = fb_ch8(((v >> 4) & 0xF) / 15.0);
            *b = fb_ch8((v & 0xF) / 15.0);
            return;
        case FB_L_RGB565:
            *r = fb_ch8(((v >> 11) & 0x1F) / 31.0);
            *g = fb_ch8(((v >> 5) & 0x3F) / 63.0);
            *b = fb_ch8((v & 0x1F) / 31.0);
            return;
        case FB_L_RGB666:
            *r = fb_ch8(((v >> 16) & 0xFC) / 252.0);
            *g = fb_ch8(((v >> 8) & 0xFC) / 252.0);
            *b = fb_ch8((v & 0xFC) / 252.0);
            return;
        case FB_L_RGB888:
            *r = (int) ((v >> 16) & 0xFF);
            *g = (int) ((v >> 8) & 0xFF);
            *b = (int) (v & 0xFF);
            return;
        case FB_L_RGBA8888:
            *r = (int) ((v >> 24) & 0xFF);
            *g = (int) ((v >> 16) & 0xFF);
            *b = (int) ((v >> 8) & 0xFF);
            *a = (int) (v & 0xFF);
            return;
        default:
            *r = *g = *b = 0;
            return;
    }
}

/* ---- given: fill and blank ------------------------------------------- */

static void fb_fill_store(const fb_format *f, unsigned char *bytes, int w, int h, zend_long v)
{
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) fb_set_word(f, bytes, w, h, x, y, v);
}

static unsigned char *fb_blank(const fb_format *f, int w, int h, size_t *size)
{
    *size = fb_bytes_for(f, w, h);
    unsigned char *bytes = emalloc(*size);
    memset(bytes, 0, *size);
    if (f->layout == FB_L_PLANAR) fb_fill_store(f, bytes, w, h, 0);   /* inverted planes start at 1 */
    return bytes;
}

/* ---- given: damage ---------------------------------------------------- */

static int fb_touches(const fb_rect *a, const fb_rect *b)
{
    return a->x <= b->x + b->w && b->x <= a->x + a->w && a->y <= b->y + b->h && b->y <= a->y + a->h;
}

static fb_rect fb_union(const fb_rect *a, const fb_rect *b)
{
    fb_rect r;
    r.x = a->x < b->x ? a->x : b->x;
    r.y = a->y < b->y ? a->y : b->y;
    int ar = a->x + a->w, br = b->x + b->w, ab = a->y + a->h, bb = b->y + b->h;
    r.w = (ar > br ? ar : br) - r.x;
    r.h = (ab > bb ? ab : bb) - r.y;
    return r;
}

/* Append r to list[0..n), absorbing everything it touches until stable; merged goes on the end. */
static void fb_merge_into(fb_rect *list, int *n, fb_rect r)
{
    int grew;
    do {
        grew = 0;
        int kept = 0;
        for (int i = 0; i < *n; i++) {
            if (fb_touches(&list[i], &r)) { r = fb_union(&list[i], &r); grew = 1; }
            else list[kept++] = list[i];
        }
        *n = kept;
    } while (grew);
    list[(*n)++] = r;
}

static void fb_touched(fb_buffer *b, fb_rect r)
{
    if (b->kind != FB_KIND_DIRTY) return;
    if (b->collapsed) { b->damage[0] = fb_union(&b->damage[0], &r); return; }
    fb_rect tmp[FB_MAX_DAMAGE + 1];
    memcpy(tmp, b->damage, sizeof(fb_rect) * b->damage_n);
    int n = b->damage_n;
    fb_merge_into(tmp, &n, r);
    if (n > FB_MAX_DAMAGE) {
        fb_rect box = tmp[0];
        for (int i = 1; i < n; i++) box = fb_union(&box, &tmp[i]);
        b->damage[0] = box; b->damage_n = 1; b->collapsed = 1;
        return;
    }
    memcpy(b->damage, tmp, sizeof(fb_rect) * n);
    b->damage_n = n;
}

static void fb_granularity_of(const fb_buffer *b, int *uw, int *uh)
{
    *uw = 1; *uh = 1;
    if (b->fmt.layout == FB_L_MONO_VP) { *uw = b->width; *uh = 8; }
    if (b->kind == FB_KIND_PAGED) { *uw = b->width; if (b->page_rows > *uh) *uh = b->page_rows; }
}

static fb_rect fb_snap(const fb_buffer *b, fb_rect r)
{
    int uw, uh; fb_granularity_of(b, &uw, &uh);
    fb_rect s;
    s.x = (r.x / uw) * uw;
    s.y = (r.y / uh) * uh;
    int x1 = ((r.x + r.w + uw - 1) / uw) * uw; if (x1 > b->width) x1 = b->width;
    int y1 = ((r.y + r.h + uh - 1) / uh) * uh; if (y1 > b->height) y1 = b->height;
    s.w = x1 - s.x; s.h = y1 - s.y;
    return s;
}

/* ---- given: store selection ------------------------------------------ */

static unsigned char *fb_read_store(fb_buffer *b)  { return b->kind == FB_KIND_RING ? b->ring[b->front] : b->bytes; }
static unsigned char *fb_write_store(fb_buffer *b) { return b->kind == FB_KIND_RING ? b->ring[b->back]  : b->bytes; }

/* Map a virtual (x, y) to store coordinates; 0 when outside the surface or the current page. */
static int fb_locate(const fb_buffer *b, int x, int y, int *sx, int *sy)
{
    if (x < 0 || y < 0 || x >= b->width || y >= b->height) return 0;
    if (b->kind == FB_KIND_PAGED) {
        int top = b->page * b->page_rows;
        if (y < top || y >= top + b->page_rows) return 0;
        y -= top;
    }
    *sx = x; *sy = y;
    return 1;
}

static int fb_store_rows(const fb_buffer *b)
{
    if (b->kind != FB_KIND_PAGED) return b->height;
    int rows = b->height - b->page * b->page_rows;
    return rows < b->page_rows ? rows : b->page_rows;
}

/* ---- entry points ------------------------------------------------------ */

zend_long fb_api_create(zval *kind, zval *pixel_format, zval *bit_depth, zval *bit_order, zval *endianness, zval *page_axis, zval *scan, zval *palette, zval *width, zval *height, zval *extra)
{
    fb_format f;
    if (!fb_resolve(&f, L(pixel_format), L(bit_depth), L(bit_order), L(endianness), L(page_axis), L(scan), palette)) return 0;
    int w = L(width), h = L(height), k = L(kind), e = L(extra);
    if (w < 1 || h < 1) return 0;
    if (k == FB_KIND_EPAPER) {
        int ok = (f.layout == FB_L_PLANAR) || (f.layout == FB_L_MONO_H && L(pixel_format) == 1) || (f.layout == FB_L_INDEX && f.palette.n > 0);
        if (!ok) return 0;
    }
    if (k == FB_KIND_PAGED && (e < 1 || (f.layout == FB_L_MONO_VP && e % 8 != 0))) return 0;
    if (k == FB_KIND_RING && e < 2) return 0;

    fb_buffer *b = ecalloc(1, sizeof *b);
    b->kind = k; b->fmt = f; b->width = w; b->height = h;
    b->store_w = w; b->store_h = h;
    if (k == FB_KIND_PAGED) {
        b->page_rows = e; b->pages = (h + e - 1) / e; b->store_h = e;
    }
    if (k == FB_KIND_RING) {
        b->frames = e; b->front = 0; b->back = 1;
        b->ring = ecalloc(e, sizeof(unsigned char *));
        for (int i = 0; i < e; i++) b->ring[i] = fb_blank(&f, w, h, &b->size);
    } else {
        b->bytes = fb_blank(&f, b->store_w, b->store_h, &b->size);
    }
    return (zend_long)(uintptr_t) b;
}

void fb_api_free(zval *h)
{
    fb_buffer *b = H(h);
    if (!b) return;
    if (b->ring) { for (int i = 0; i < b->frames; i++) efree(b->ring[i]); efree(b->ring); }
    if (b->bytes) efree(b->bytes);
    efree(b);
}

zend_long fb_api_width(zval *h)  { return H(h)->width; }
zend_long fb_api_height(zval *h) { return H(h)->height; }
zend_long fb_api_size(zval *h)
{
    fb_buffer *b = H(h);
    return (zend_long) fb_bytes_for(&b->fmt, b->store_w, fb_store_rows(b));
}

zend_long fb_api_get(zval *h, zval *x, zval *y)
{
    fb_buffer *b = H(h); int sx, sy;
    if (!fb_locate(b, L(x), L(y), &sx, &sy)) return 0;
    return fb_get_word(&b->fmt, fb_read_store(b), b->store_w, b->store_h, sx, sy);
}

static void fb_set_one(fb_buffer *b, int x, int y, zend_long v)
{
    int sx, sy;
    if (!fb_locate(b, x, y, &sx, &sy)) return;
    fb_set_word(&b->fmt, fb_write_store(b), b->store_w, b->store_h, sx, sy, v);
    fb_rect r = { x, y, 1, 1 };
    fb_touched(b, r);
}

void fb_api_set(zval *h, zval *x, zval *y, zval *v) { fb_set_one(H(h), L(x), L(y), Z_LVAL_P(v)); }

void fb_api_set_pixels(zval *h, zval *pixels)
{
    fb_buffer *b = H(h); zval *p;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(pixels), p) {
        if (Z_TYPE_P(p) != IS_ARRAY) continue;
        zval *px = zend_hash_index_find(Z_ARRVAL_P(p), 0), *py = zend_hash_index_find(Z_ARRVAL_P(p), 1), *pv = zend_hash_index_find(Z_ARRVAL_P(p), 2);
        if (px && py && pv) fb_set_one(b, (int) Z_LVAL_P(px), (int) Z_LVAL_P(py), Z_LVAL_P(pv));
    } ZEND_HASH_FOREACH_END();
}

void fb_api_set_segment(zval *h, zval *x, zval *y, zval *w, zval *hh, zval *v)
{
    fb_buffer *b = H(h);
    int x0 = L(x), y0 = L(y), x1 = x0 + L(w), y1 = y0 + L(hh);
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > b->width) x1 = b->width; if (y1 > b->height) y1 = b->height;
    if (b->kind == FB_KIND_PAGED) {
        int top = b->page * b->page_rows, bottom = top + fb_store_rows(b);
        if (y0 < top) y0 = top; if (y1 > bottom) y1 = bottom;
    }
    if (x1 <= x0 || y1 <= y0) return;
    unsigned char *store = fb_write_store(b);
    for (int yy = y0; yy < y1; yy++) {
        int sx, sy; fb_locate(b, x0, yy, &sx, &sy);
        for (int xx = 0; xx < x1 - x0; xx++) fb_set_word(&b->fmt, store, b->store_w, b->store_h, sx + xx, sy, Z_LVAL_P(v));
    }
    fb_rect r = { x0, y0, x1 - x0, y1 - y0 };
    fb_touched(b, r);
}

void fb_api_fill(zval *h, zval *v)
{
    fb_buffer *b = H(h);
    fb_fill_store(&b->fmt, fb_write_store(b), b->store_w, b->store_h, Z_LVAL_P(v));
    fb_rect r = { 0, 0, b->width, b->height };
    fb_touched(b, r);
}

/* Sub-rect of the read store into a fresh buffer of the same format, rows reversed when scan = 1. Returns emalloc'd bytes. */
static unsigned char *fb_region_bytes(fb_buffer *b, const fb_format *out_f, int rx, int ry, int rw, int rh, int scan, size_t *size)
{
    unsigned char *out = fb_blank(out_f, rw, rh, size);
    const unsigned char *store = fb_read_store(b);
    for (int y = 0; y < rh; y++) {
        int yy = scan == 1 ? rh - 1 - y : y;
        for (int x = 0; x < rw; x++) {
            zend_long word = fb_get_word(&b->fmt, store, b->store_w, b->store_h, rx + x, ry + y);
            if (out_f == &b->fmt) {
                fb_set_word(out_f, out, rw, rh, x, yy, word);
            } else {
                int r, g, bb, a; fb_unmap(&b->fmt, word, &r, &g, &bb, &a);
                fb_set_word(out_f, out, rw, rh, x, yy, fb_map(out_f, r, g, bb, a));
            }
        }
    }
    return out;
}

void fb_api_bytes(zval *return_value, zval *h)
{
    fb_buffer *b = H(h);
    int rows = fb_store_rows(b);
    if (rows == b->store_h) { ZVAL_STRINGL(return_value, (char *) fb_read_store(b), b->size); return; }
    size_t size; unsigned char *out = fb_region_bytes(b, &b->fmt, 0, 0, b->store_w, rows, 0, &size);
    ZVAL_STRINGL(return_value, (char *) out, size); efree(out);
}

void fb_api_region(zval *return_value, zval *h, zval *x, zval *y, zval *w, zval *hh, zval *scan)
{
    fb_buffer *b = H(h);
    int rx = L(x), ry = L(y), rw = L(w), rh = L(hh);
    if (b->kind == FB_KIND_PAGED) {
        int top = b->page * b->page_rows, bottom = top + fb_store_rows(b);
        int y0 = ry < top ? top : ry, y1 = ry + rh > bottom ? bottom : ry + rh;
        if (y1 <= y0 || rw < 1) { ZVAL_EMPTY_STRING(return_value); return; }
        ry = y0 - top; rh = y1 - y0;
    }
    size_t size; unsigned char *out = fb_region_bytes(b, &b->fmt, rx, ry, rw, rh, L(scan), &size);
    ZVAL_STRINGL(return_value, (char *) out, size); efree(out);
}

void fb_api_layer(zval *return_value, zval *h, zval *layer)
{
    fb_buffer *b = H(h); int k = L(layer);
    const unsigned char *store = fb_read_store(b);
    if (b->fmt.layout != FB_L_PLANAR || k < 0) { ZVAL_STRINGL(return_value, (char *) store, b->size); return; }
    size_t plane = ((b->store_w + 7) / 8) * b->store_h;
    ZVAL_STRINGL(return_value, (char *) store + plane * k, plane);
}

zend_long fb_api_pointer(zval *h) { return (zend_long)(uintptr_t) fb_read_store(H(h)); }

void fb_api_to_rgba8(zval *return_value, zval *h)
{
    fb_buffer *b = H(h);
    int rows = fb_store_rows(b);
    size_t size = (size_t) b->store_w * rows * 4;
    unsigned char *out = emalloc(size), *p = out;
    const unsigned char *store = fb_read_store(b);
    for (int y = 0; y < rows; y++) for (int x = 0; x < b->store_w; x++) {
        int r, g, bb, a; fb_unmap(&b->fmt, fb_get_word(&b->fmt, store, b->store_w, b->store_h, x, y), &r, &g, &bb, &a);
        *p++ = r; *p++ = g; *p++ = bb; *p++ = a;
    }
    ZVAL_STRINGL(return_value, (char *) out, size); efree(out);
}

void fb_api_blit_rgba8(zval *h, zval *rgba8, zval *sw, zval *sh, zval *ox, zval *oy)
{
    fb_buffer *b = H(h);
    const unsigned char *src = (const unsigned char *) Z_STRVAL_P(rgba8);
    int w = L(sw), hh = L(sh);
    if (Z_STRLEN_P(rgba8) < (size_t) w * hh * 4) return;
    for (int y = 0; y < hh; y++) for (int x = 0; x < w; x++) {
        const unsigned char *px = src + ((size_t) y * w + x) * 4;
        fb_set_one(b, x + L(ox), y + L(oy), fb_map(&b->fmt, px[0], px[1], px[2], px[3]));
    }
}

void fb_api_transcode(zval *return_value, zval *h, zval *pixel_format, zval *bit_depth, zval *bit_order, zval *endianness, zval *page_axis, zval *scan, zval *palette, zval *x, zval *y, zval *w, zval *hh)
{
    fb_buffer *b = H(h); fb_format out_f;
    if (!fb_resolve(&out_f, L(pixel_format), L(bit_depth), L(bit_order), L(endianness), L(page_axis), L(scan), palette)) { ZVAL_EMPTY_STRING(return_value); return; }
    int rx = L(x), ry = L(y), rw = L(w), rh = L(hh);
    if (b->kind == FB_KIND_PAGED) {
        int top = b->page * b->page_rows, bottom = top + fb_store_rows(b);
        int y0 = ry < top ? top : ry, y1 = ry + rh > bottom ? bottom : ry + rh;
        if (y1 <= y0) { ZVAL_EMPTY_STRING(return_value); return; }
        ry = y0 - top; rh = y1 - y0;
    }
    size_t size; unsigned char *out = fb_region_bytes(b, &out_f, rx, ry, rw, rh, out_f.scan, &size);
    ZVAL_STRINGL(return_value, (char *) out, size); efree(out);
}

void fb_api_granularity(zval *return_value, zval *h)
{
    int uw, uh; fb_granularity_of(H(h), &uw, &uh);
    array_init(return_value); add_next_index_long(return_value, uw); add_next_index_long(return_value, uh);
}

void fb_api_begin_epoch(zval *h) { fb_buffer *b = H(h); b->damage_n = 0; b->collapsed = 0; }

void fb_api_damage(zval *return_value, zval *h)
{
    fb_buffer *b = H(h);
    array_init(return_value);
    if (b->kind != FB_KIND_DIRTY) return;
    fb_rect snapped[FB_MAX_DAMAGE + 1]; int n = 0;
    for (int i = 0; i < b->damage_n; i++) fb_merge_into(snapped, &n, fb_snap(b, b->damage[i]));
    for (int i = 0; i < n; i++) {
        zval r; array_init(&r);
        add_next_index_long(&r, snapped[i].x); add_next_index_long(&r, snapped[i].y);
        add_next_index_long(&r, snapped[i].w); add_next_index_long(&r, snapped[i].h);
        add_next_index_zval(return_value, &r);
    }
}

void fb_api_set_page(zval *h, zval *page)
{
    fb_buffer *b = H(h); int p = L(page);
    if (b->kind != FB_KIND_PAGED || p < 0 || p >= b->pages) return;
    b->page = p;
    memset(b->bytes, 0, b->size);
    if (b->fmt.layout == FB_L_PLANAR) fb_fill_store(&b->fmt, b->bytes, b->store_w, b->store_h, 0);
}

zend_long fb_api_page(zval *h)  { return H(h)->page; }
zend_long fb_api_pages(zval *h) { return H(h)->pages; }

void fb_api_present(zval *h)
{
    fb_buffer *b = H(h);
    if (b->kind != FB_KIND_RING) return;
    b->front = b->back; b->back = (b->back + 1) % b->frames;
}

zend_long fb_api_frames(zval *h) { return H(h)->frames; }
