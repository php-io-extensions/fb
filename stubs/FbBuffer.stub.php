<?php

/** @generate-class-entries */

/**
 * One surface's pixels in one FbFormat, held in C. Coordinates outside the
 * buffer are refused with ValueError; a list with any pixel outside is refused
 * whole, with nothing written.
 *
 * @not-serializable
 */
final class FbBuffer
{
    public function __construct(FbFormat $format, int $width, int $height) {}

    public function format(): FbFormat {}

    public function width(): int {}

    public function height(): int {}

    /** Bytes held. */
    public function size(): int {}

    public function get(int $x, int $y): int {}

    public function set(int $x, int $y, int $value): void {}

    /**
     * @param array $pixels One [int $x, int $y, int $value] per pixel.
     * @return array|null [x, y, width, height] of the bounding box written; null for an empty list.
     */
    public function setPixels(array $pixels): ?array {}

    /**
     * @param array $coordinates One [int $x, int $y] per pixel.
     * @return array|null [x, y, width, height] of the bounding box written; null for an empty list.
     */
    public function setCoordinates(array $coordinates, int $value): ?array {}

    /** Fill a rect that lies inside the buffer. */
    public function rect(int $x, int $y, int $width, int $height, int $value): void {}

    public function fill(int $value): void {}

    /** The raw bytes, top row first. */
    public function bytes(): string {}

    /** One plane of a planar buffer; layer 0 of any other layout is all of it. */
    public function layer(int $layer): string {}

    /**
     * A rect's bytes, rows in the format's scan order. Null copies the words
     * as stored, in this buffer's own format; another format maps every pixel
     * through RGBA8.
     */
    public function region(int $x, int $y, int $width, int $height, ?FbFormat $format = null): string {}

    /** RGBA8 bytes, top-left first. */
    public function rgba8(): string {}

    /**
     * Map RGBA8 pixels into the buffer at (x, y), clipped to the buffer and to the clip rect.
     *
     * @param int|null $clipWidth Null clips at the buffer's right edge.
     * @param int|null $clipHeight Null clips at the buffer's bottom edge.
     * @return array|null [x, y, width, height] written; null when nothing landed.
     */
    public function blitRgba8(string $rgba8, int $width, int $height, int $x, int $y, int $clipX = 0, int $clipY = 0, ?int $clipWidth = null, ?int $clipHeight = null): ?array {}

    /** Copy a rect from a buffer of the same size and storage, words as stored. */
    public function copy(FbBuffer $source, int $x, int $y, int $width, int $height): void {}

    /** One bit per pixel, rows padded to a byte, x0 in bit 7: set where the pixel's word equals $value. */
    public function plane(int $value): string {}

    /**
     * Paint coverage spans in one colour. $spans holds 7-byte records,
     * little-endian: y, x, length (uint16) and coverage (uint8). Effective
     * alpha is alpha × coverage; RGB and grey layouts blend source-over,
     * mono, palette and planar layouts write the colour where it reaches 128.
     * Every span is checked before any is painted.
     *
     * @param int $rgba 0xRRGGBBAA
     * @return array|null [x, y, width, height] of the spans; null for none.
     */
    public function paintSpans(string $spans, int $rgba): ?array {}

    /**
     * Paint coverage masks in one colour, in order, each cut to the clip.
     * A mask is [string $coverage, int $width, int $left, int $top]: rows of
     * $width coverage bytes, top-down, its top-left pixel at ($left, $top),
     * which may lie off the buffer (within ±2^30). Each pixel of coverage
     * above 0 is painted as a span of that coverage is by paintSpans(). The
     * clip must be a non-empty rect inside the buffer; every mask is checked
     * before any is painted.
     *
     * @param array $masks a list of [coverage, width, left, top]
     * @param int $rgba 0xRRGGBBAA
     * @return array|null [x, y, width, height] of the pixels with coverage; null for none.
     */
    public function paintMasks(array $masks, int $rgba, int $clipX, int $clipY, int $clipWidth, int $clipHeight): ?array {}

    /**
     * Replace a region's pixels with the result of a filter chain run over
     * them (straight RGBA8 in and out; the steps work on premultiplied values,
     * premultiplying by (c × a + 127) / 255 and straightening by
     * (c × 255 + a / 2) / a). Each step is a list, its kind first: blur,
     * matrix, flood, composite, blend, noise, displace, field or table (the layouts are
     * Surface's FilterSteps). A step reads the result of an earlier step by
     * name, "source" being the region's pixels, or the previous result when it
     * names none. Noise is a function of the position on the surface, y
     * counted from $row. Every step is checked before any pixel is read.
     *
     * @param array $steps
     * @param array $maps Displacement maps, [width, height, straight RGBA8 bytes] each.
     * @param int $row The row of a larger surface this buffer's row 0 stands for (0..65535).
     * @param bool $premultiplied The region's pixels are premultiplied RGBA8 already (a layer drawn over nothing): they are the source as they are; the result is written straight either way.
     */
    public function filter(array $steps, array $maps, int $x, int $y, int $width, int $height, int $row = 0, bool $premultiplied = false): void {}

    /**
     * Paint a gradient through spans (7 bytes each: y, x, length uint16 LE,
     * coverage uint8; all inside the buffer, checked before any is painted).
     * The point (u, v) = (a·x + c·y + e, b·x + d·y + f) at a pixel's centre
     * gives t = u, or hypot(u, v) when $radial; $spread folds t into 0..1; the
     * colour is $ramp[floor(t × 255 + ½)]. u and v are worked out at a span's
     * first pixel and stepped along it. Blended like a span, alpha = ramp
     * alpha × coverage.
     *
     * @param string $ramp 256 colours of 4 bytes, RGBA, straight alpha.
     * @param int $spread 0 pad, 1 repeat, 2 reflect.
     * @param array $inverse [a, b, c, d, e, f], six finite numbers.
     * @param int $row The row of a larger surface this buffer's row 0 stands for (0..65535): y counts from there.
     * @return array|null [x, y, width, height] of the spans; null for none.
     */
    public function paintGradient(string $spans, string $ramp, bool $radial, int $spread, array $inverse, int $row = 0): ?array {}

    /**
     * Paint RGBA8 pixels (straight alpha) through an inverse placement. For
     * each pixel of the target rect, which must lie inside the buffer, the
     * point (u, v) = (a·x + c·y + e, b·x + d·y + f) at the pixel's centre picks
     * the source pixel when it lies inside the image: the one under it, or with
     * $smooth the four around it (colours weighed by alpha, edges held). It is
     * blended like a span, with alpha = source alpha × opacity. Each sampled
     * pixel first takes $tint (alpha too) and, last, the coverage of its mask
     * span, each (x × y + 127) / 255.
     *
     * @param array $inverse [a, b, c, d, e, f], six finite numbers.
     * @param int $opacity 0..255
     * @param int $row The row of a larger surface this buffer's row 0 stands for (0..65535): y counts from there.
     * @param int $tint 0xRRGGBBAA multiplied into every sampled pixel, alpha included; 0xFFFFFFFF changes nothing.
     * @param array|null $window [u0, v0, u1, v1] in image pixels, u0 < u1 and v0 < v1: only pixels whose point lies inside it are painted (smooth sampling still reads the whole image).
     * @param string|null $coverage Spans (7 bytes each: y, x, length uint16 LE, coverage uint8) in this buffer's rows, rows ascending and x ascending without overlap, all inside the buffer: a pixel's alpha is scaled by its span's coverage, and a pixel no span holds is not painted.
     */
    public function paintRgba8(string $rgba8, int $width, int $height, array $inverse, int $x, int $y, int $targetWidth, int $targetHeight, int $opacity = 255, bool $smooth = false, int $row = 0, int $tint = 0xFFFFFFFF, ?array $window = null, ?string $coverage = null): void {}

    /**
     * One level down of a dual-filter (Kawase) pyramid: this RGBA8888 buffer, ceil(width / 2) x ceil(height / 2), from
     * the region of an RGBA8888 $source: each pixel (4 x the 2x2 centre + the 4x4 block + 16) >> 5, edges held.
     */
    public function kawaseDown(FbBuffer $source, int $x, int $y, int $width, int $height): void {}

    /** One level up: this RGBA8888 buffer, at most twice $source on each side, from it by the 8-tap tent, (sum + 96) / 192. */
    public function kawaseUp(FbBuffer $source): void {}

    /**
     * A glass pane over this buffer, inside the clip (inside the buffer): the rounded box sampled from $blur (an RGBA8888
     * level $scale target pixels a pixel, from target pixel (originX, originY)), bent at the rim, shaded and blended.
     * The arithmetic is core.h's fb_paint_glass().
     *
     * @param array $box [x0, y0, x1, y1]
     * @param array $radii [tl, tr, br, bl], held to 0..min(w, h)/2
     * @param array $look [frost, refraction, bevel, saturation, light, grain, tint 0xRRGGBBAA, rim 0xRRGGBBAA, alpha 0..255]
     * @return array|null [x, y, width, height] of what was painted; null for nothing.
     */
    public function paintGlass(array $box, array $radii, FbBuffer $blur, int $scale, ?FbBuffer $sharp, int $originX, int $originY, array $look, int $clipX, int $clipY, int $clipWidth, int $clipHeight): ?array {}

    /** The address of the bytes, valid while this object lives. */
    public function pointer(): int {}

    /** @return array [unitWidth, unitHeight]: the smallest block of pixels whose bytes stand alone. */
    public function granularity(): array {}
}
