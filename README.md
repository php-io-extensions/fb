# ext-fb

Pixel storage in C. `FbFormat` says how pixels are stored; `FbBuffer` holds one
surface in that format and reads, writes, converts and copies it. Written
directly in C against the Zend API, with no dependency: it knows nothing of any
framework. PHP 8.4+, NTS and ZTS, Linux and macOS.

## Formats

| Layout | Bytes | Pixel word |
|---|---|---|
| `FB_LAYOUT_MONO_ROWS` | 1 bit per pixel, rows padded to a byte | 0 / 1 (1 is lit, white) |
| `FB_LAYOUT_MONO_PAGES` | one byte per column per 8-row page (SSD1306) | 0 / 1 |
| `FB_LAYOUT_INDEX2`, `_INDEX4`, `_INDEX8` | 2, 4 or 8 bits per pixel, rows padded | a grey level, or a palette code |
| `FB_LAYOUT_RGB444` | two pixels in three bytes | `0xRGB` |
| `FB_LAYOUT_RGB565` | two bytes | RGB565 |
| `FB_LAYOUT_RGB666` | three bytes, six bits left-aligned in each | `0xRRGGBB`, low two bits of each channel zero |
| `FB_LAYOUT_RGB888` | three bytes | `0xRRGGBB` |
| `FB_LAYOUT_RGBA8888` | four bytes | `0xRRGGBBAA` |
| `FB_LAYOUT_PLANAR` | one 1-bit plane per ink | a mask of inks; 0 is paper |

Orders: `FB_BIT_ORDER_MSB` / `_LSB` (mono, index, planar), `FB_BYTE_ORDER_MSB` /
`_LSB` (RGB565), `FB_CHANNELS_RGB` / `_BGR` (12 to 24 bits) and `_RGBA` /
`_BGRA` / `_ARGB` / `_ABGR` (32 bits), `FB_SCAN_TOP_DOWN` / `_BOTTOM_UP` (the
row order a region is emitted in). The word never changes with an order.

A palette is a list of `[int $rgb, bool $inverted, int $code]`, at most 16: the
colour as `0xRRGGBB`, whether a planar plane stores the ink as 0, and the word
an index layout stores for it. Planar needs one; an index layout without one
holds grey levels.

Colours map with integer arithmetic on 0..255 channels: Rec. 709 luma for mono
(lit at half) and grey, the nearest palette entry for index and planar (the
earlier entry wins a tie), bit packing for RGB.

## Example

```php
$format = new FbFormat(FB_LAYOUT_RGB565);
$buffer = new FbBuffer($format, 240, 135);

$buffer->fill(0x0000);
$buffer->rect(20, 20, 100, 50, 0xF800);
$buffer->set(0, 0, $format->mapRgba8(255, 128, 0));

$wire = $buffer->bytes();                                    // as stored, top row first
$part = $buffer->region(20, 20, 100, 50);                    // a rect, as stored
$bgra = $buffer->region(0, 0, 240, 135, new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_BGRA));
$address = $buffer->pointer();                               // valid while $buffer lives
```

`paintSpans($spans, 0xRRGGBBAA)` paints coverage spans, 7 bytes each
(`y`, `x`, `length` as uint16, `coverage` as uint8, little-endian), the format a
rasteriser such as ext-rasterize answers: RGB and grey layouts blend
source-over by alpha × coverage, mono, palette and planar layouts write the
colour where that reaches half.

Coordinates outside a buffer are a `ValueError`. A pixel list with any entry
outside is refused whole, with nothing written. The stubs in `stubs/` are the
full declaration.

## Install

```bash
./install-macos.sh            # Homebrew php@8.4 and php@8.4-zts, or pass PHP binaries
./install-debian-trixie.sh    # Debian trixie, Raspberry Pi OS
```

## Test

```bash
composer install && php vendor/bin/pest
```
