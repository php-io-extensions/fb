<?php

/** @generate-class-entries */

/**
 * How pixels are stored: a byte layout, its bit, byte and channel orders, the
 * row order a region is emitted in, and the palette of an index or planar
 * layout. Immutable.
 *
 * @not-serializable
 */
final class FbFormat
{
    /**
     * @param array $palette One [int $rgb, bool $inverted, int $code] per ink, at most 16: the colour as 0xRRGGBB,
     *                       whether a planar plane stores the ink as 0, and the word an index layout stores for it.
     *                       Required by FB_LAYOUT_PLANAR; an index layout without one holds grey levels.
     */
    public function __construct(int $layout, int $bitOrder = FB_BIT_ORDER_MSB, int $byteOrder = FB_BYTE_ORDER_MSB, int $channelOrder = FB_CHANNELS_RGB, int $scan = FB_SCAN_TOP_DOWN, array $palette = []) {}

    public function layout(): int {}

    public function bitOrder(): int {}

    public function byteOrder(): int {}

    public function channelOrder(): int {}

    public function scan(): int {}

    /** Bytes a width x height surface takes. */
    public function size(int $width, int $height): int {}

    /** 0..255 channels to this format's pixel word. */
    public function mapRgba8(int $red, int $green, int $blue, int $alpha = 255): int {}

    /** A pixel word as 0xRRGGBBAA. */
    public function unmapRgba8(int $word): int {}
}
