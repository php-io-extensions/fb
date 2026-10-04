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
     * Paint RGBA8 pixels (straight alpha) through an inverse placement. For
     * each pixel of the target rect, which must lie inside the buffer, the
     * point (u, v) = (a·x + c·y + e, b·x + d·y + f) at the pixel's centre picks
     * the source pixel when it lies inside the image: the one under it, or with
     * $smooth the four around it (colours weighed by alpha, edges held). It is
     * blended like a span, with alpha = source alpha × opacity.
     *
     * @param array $inverse [a, b, c, d, e, f], six finite numbers.
     * @param int $opacity 0..255
     * @param int $row The row of a larger surface this buffer's row 0 stands for (0..65535): y counts from there.
     */
    public function paintRgba8(string $rgba8, int $width, int $height, array $inverse, int $x, int $y, int $targetWidth, int $targetHeight, int $opacity = 255, bool $smooth = false, int $row = 0): void {}

    /** The address of the bytes, valid while this object lives. */
    public function pointer(): int {}

    /** @return array [unitWidth, unitHeight]: the smallest block of pixels whose bytes stand alone. */
    public function granularity(): array {}
}
