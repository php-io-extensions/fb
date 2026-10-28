<div align="center" id="header">

<h1>ext-fb</h1>

<h3>Draw pixel data in C-land in the shape your IC Panels and Video Drivers prefer. With PHP.</h3>

[![CI](https://img.shields.io/github/actions/workflow/status/php-io-extensions/fb/ci.yml?style=flat-square&label=CI)](https://github.com/php-io-extensions/fb/actions/workflows/ci.yml)
[![Packagist](https://img.shields.io/packagist/v/php-io-extensions/fb?style=flat-square)](https://packagist.org/packages/php-io-extensions/fb)
[![PHP](https://img.shields.io/packagist/dependency-v/php-io-extensions/fb/php?style=flat-square&label=PHP)](https://packagist.org/packages/php-io-extensions/fb)
[![License](https://img.shields.io/github/license/php-io-extensions/fb?style=flat-square)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-linux_%7C_macos-blue?style=flat-square&logo=linux&logoColor=white)](#requirements)
[![OKF](https://img.shields.io/badge/OKF-0.2-6f42c1?style=flat-square&label=knowledge%20bundle)](.okf/index.md)
[![PIE](https://img.shields.io/badge/PIE-installable-4f5b93?style=flat-square&logo=php&logoColor=white)](https://github.com/php/pie)

</div>

Every display takes its pixels in its own byte layout. A typical I2C OLED eats pixel data shaped by the binary bit, packed into columns rows each sized at 1 byte. A common TFT panel eat pixels that are 2 bytes in-size. A Linux framebuffer wants 32-bit pixels, that's 4 bytes per pixel. A colored e-paper display likes it's pixels shaped similarly to an OLED but in multiple channels. Drawing for any of them means keeping the picture in that exact layout, changing pixels in it, and sending all of it, or only the part that changed, down the wire.

The easiest way to abstract this in PHP is with a string. While that is the most developer-friendly way to learn how framebuffers works, a string is a heavier primitive type in PHP, and requires extra work to turn in into a blob of bytes. Sometimes, it takes more work than allowed to keep up with even 30FPS.
For most framebuffers, ext-gd wouldn't be helpful either, you would have to transcode its output which we want to avoid for performance. 

<b>The solution?</b> Use an extension that manages the framebuffer in C land. Now its C-shaped blob that can be manipulated in PHP-land, that's extra performant.
That's where <b>ext-fb</b> comes in. It holds the pixels in C. Describe the pixel format you want to work with, and draw pixels to it.
When you're ready, flush the buffer and get a blog of pixels ready to be fed into an I2C/SPI Panel, A kernel-level framebuffer or a graphics API.

## Basic usage

Most ways you will interface with ext-fb are in two parts -

Describe the layout with an `FbFormat`, make an `FbBuffer` in it, and draw. A pixel value is a word in the layout's own terms, here an RGB565 word; `mapRgba8()` turns 0..255 channels into one.

```php
// An ST7789 panel: 240 × 135 pixels of RGB565, high byte first on the wire.
$format = new FbFormat(FB_LAYOUT_RGB565);
$screen = new FbBuffer($format, 240, 135);

$black = $format->mapRgba8(0, 0, 0);
$red = $format->mapRgba8(255, 0, 0);

$screen->fill($black);
$screen->rect(20, 20, 100, 50, $red);
$screen->set(0, 0, $format->mapRgba8(255, 128, 0));
```

Take the bytes out. `bytes()` is the whole store, top row first, exactly as the panel takes it. `region()` is one rect of it, for a partial update. Hand either to whatever talks to the device.

```php
$frame = $screen->bytes();
$box = $screen->region(20, 20, 100, 50);

printf("frame %d bytes, box %d bytes\n", strlen($frame), strlen($box));
printf("pixel (20, 20) = 0x%04X, first two bytes %s\n", $screen->get(20, 20), bin2hex(substr($box, 0, 2)));
printf("pixel (0, 0) as RGBA8 = 0x%08X\n", $format->unmapRgba8($screen->get(0, 0)));
```

```console
$ php panel.php
frame 64800 bytes, box 10000 bytes
pixel (20, 20) = 0xF800, first two bytes f800
pixel (0, 0) as RGBA8 = 0xFF8200FF
```

The orange went in as `(255, 128, 0)` and comes back as `0xFF8200`: RGB565 keeps six bits of green, and the word is what the panel will show.

### Examples

#### Drive an SSD1306 page by page

The SSD1306 stores its 128 × 64 pixels as eight pages of 128 column bytes, the top row of each page in bit 0. `setCoordinates()` answers the box it touched, and `granularity()` says the smallest block whose bytes stand alone, so a partial update can be rounded out to whole pages.

```php
$oled = new FbBuffer(new FbFormat(FB_LAYOUT_MONO_PAGES, bitOrder: FB_BIT_ORDER_LSB), 128, 64);

$dirty = $oled->setCoordinates([[3, 1], [4, 1], [5, 9]], 1);
echo json_encode($dirty), ' ', json_encode($oled->granularity()), "\n";

echo strlen($oled->bytes()), " bytes; page 0, column 3 = 0x", bin2hex($oled->bytes()[3]), "\n";
echo strlen($oled->region(0, 0, 128, 16)), " bytes for pages 0 and 1\n";
```

```console
[3,1,3,9] [128,8]
1024 bytes; page 0, column 3 = 0x02
256 bytes for pages 0 and 1
```

#### Paint shapes from coverage spans

`paintSpans()` takes 7-byte spans, `y`, `x` and `length` as uint16 and `coverage` as uint8, little-endian: the bytes [ext-rasterize](https://github.com/php-io-extensions/rasterize) answers for paths, ellipses, rings and lines. RGB and grey layouts blend the colour source-over by alpha × coverage; mono, palette and planar layouts write it where that reaches half.

```php
$disc = (new RasterScanner(0, 0, 64, 64, true))->ellipse(32, 32, 20, 20);

$buffer = new FbBuffer(new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA), 64, 64);
$buffer->fill(0x000000FF);

echo json_encode($buffer->paintSpans($disc, 0xFF8000FF)), "\n";
printf("centre 0x%08X, rim 0x%08X, outside 0x%08X\n", $buffer->get(32, 32), $buffer->get(12, 30), $buffer->get(0, 0));
```

```console
[12,12,40,40]
centre 0xFF8000FF, rim 0xF07800FF, outside 0x000000FF
```

#### Place an image scaled

`paintRgba8()` places an RGBA8 image through an inverse placement: six numbers `[a, b, c, d, e, f]` that map the centre of each target pixel `(x, y)` back to the image point `(a·x + c·y + e, b·x + d·y + f)`. Scaling by four is `[0.25, 0, 0, 0.25, 0, 0]`; a rotation or a shear is the inverse of its matrix. Pass `$smooth` to sample the four source pixels around each point instead of the nearest one.

```php
// A 2 × 2 image: red, green / blue, white.
$image = pack('N*', 0xFF0000FF, 0x00FF00FF, 0x0000FFFF, 0xFFFFFFFF);

$buffer = new FbBuffer(new FbFormat(FB_LAYOUT_RGB888), 8, 8);
$buffer->paintRgba8($image, 2, 2, [0.25, 0, 0, 0.25, 0, 0], 0, 0, 8, 8);

printf("%06X %06X %06X %06X\n", $buffer->get(0, 0), $buffer->get(7, 0), $buffer->get(0, 7), $buffer->get(7, 7));
```

```console
FF0000 00FF00 0000FF FFFFFF
```

#### Convert for another consumer

`region()` with a second format maps every pixel of the rect through RGBA8 into it. One buffer can feed a panel in its own layout and, from the same pixels, a texture, a PNG encoder, or a 32-bit framebuffer that stores blue first. `$screen` is the RGB565 panel buffer from Basic usage.

```php
$bgra = $screen->region(0, 0, 240, 135, new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_BGRA));
$rgba = $screen->rgba8();   // the same pixels as RGBA8, top-left first
```

## Requirements

| | |
|---|---|
| OS | Linux or macOS. `composer.json` lists those two families. |
| PHP | 8.4 or newer, NTS or ZTS, 64-bit or 32-bit. On a 32-bit PHP a buffer's sides stop where its bytes would no longer fit a `zend_long`: about 23,166 square. |
| Libraries | None. The extension is plain C on the Zend API. |

To build it you need a C compiler and the development files of the PHP you intend to load it into:

```bash
# Debian, Ubuntu, Raspberry Pi OS
sudo apt install build-essential php8.4-dev

# macOS: Xcode Command Line Tools, and Homebrew's PHP ships phpize and php-config
xcode-select --install
brew install php@8.4
```

## Installation

### With PHP PIE (recommended)

[PIE](https://github.com/php/pie) is the PHP Foundation's installer for extensions. It downloads the tagged source from Packagist, builds it against the PHP on your `PATH`, installs the module and enables it. `sudo` where your PHP's extension directory is root-owned, as on Debian; not on a Homebrew Mac:

```bash
pie install php-io-extensions/fb
```

### Manually (install script)

The repository ships two installers, one per platform. Each one builds for each PHP it targets, installs `fb.so` into that PHP's `extension_dir`, writes `30-fb.ini` into its `conf.d` scan directory, and checks the module loads. The macOS one builds in a temporary copy of the checkout and ad-hoc signs the module so the system loads it on first use; the Debian one builds in the checkout, writes the ini into the CLI, FPM and Apache `conf.d` of that PHP, and cleans the build products out afterwards.

```bash
git clone https://github.com/php-io-extensions/fb.git
cd fb

./install-debian-trixie.sh             # Debian Trixie, Ubuntu 24.04+, Raspberry Pi OS: the php on PATH
./install-macos.sh                     # Homebrew php@8.4 and php@8.4-zts
./install-macos.sh /path/to/bin/php    # the macOS script takes specific PHP binaries
```

### Manually (phpize)

```bash
git clone https://github.com/php-io-extensions/fb.git
cd fb
phpize
./configure --enable-fb
make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu)"
sudo make install
```

Then enable it: add `extension=fb.so` to your `php.ini`, or drop that line into a file in the scan directory that `php --ini` reports.

### Verify

```console
$ php --ri fb

fb

fb support => enabled
Version => 0.11.x
```

## API

Two final classes and 23 constants, declared in [`stubs/`](stubs). Coordinates count from the top-left pixel. A coordinate outside the buffer, a rect that is empty or not inside it, or a malformed argument throws `ValueError`, and a list is checked whole before anything is written, so a refused call leaves the buffer as it was. Neither class can be cloned or serialized, and calling `__construct()` a second time throws `Error`.

### Constants

| Layout | Bytes | Pixel word |
|---|---|---|
| `FB_LAYOUT_MONO_ROWS` | 1 bit per pixel, rows padded to a byte | `0` or `1`, where 1 is lit, white |
| `FB_LAYOUT_MONO_PAGES` | one byte per column per 8-row page, as an SSD1306 stores it | `0` or `1` |
| `FB_LAYOUT_INDEX2`, `FB_LAYOUT_INDEX4`, `FB_LAYOUT_INDEX8` | 2, 4 or 8 bits per pixel, rows padded | a grey level, or a palette code |
| `FB_LAYOUT_RGB444` | two pixels in three bytes | `0xRGB` |
| `FB_LAYOUT_RGB565` | two bytes | RGB565 |
| `FB_LAYOUT_RGB666` | three bytes, six bits left-aligned in each | `0xRRGGBB`, the low two bits of each channel zero |
| `FB_LAYOUT_RGB888` | three bytes | `0xRRGGBB` |
| `FB_LAYOUT_RGBA8888` | four bytes | `0xRRGGBBAA` |
| `FB_LAYOUT_PLANAR` | one 1-bit plane per ink | a mask of inks; `0` is paper |

| Order | Applies to | Values |
|---|---|---|
| Bit order | mono, index and planar layouts | `FB_BIT_ORDER_MSB` (the default: the leftmost or topmost pixel in bit 7), `FB_BIT_ORDER_LSB` |
| Byte order | `FB_LAYOUT_RGB565` | `FB_BYTE_ORDER_MSB` (the default), `FB_BYTE_ORDER_LSB` |
| Channel order | RGB layouts | `FB_CHANNELS_RGB` (the default), `FB_CHANNELS_BGR` for 12 to 24 bits; `FB_CHANNELS_RGBA`, `FB_CHANNELS_BGRA`, `FB_CHANNELS_ARGB`, `FB_CHANNELS_ABGR` for `FB_LAYOUT_RGBA8888`, which needs one of them |
| Scan | the row order `region()` emits | `FB_SCAN_TOP_DOWN` (the default), `FB_SCAN_BOTTOM_UP` |

An order changes the bytes, never the word: an `FB_LAYOUT_RGBA8888` pixel is `0xRRGGBBAA` to `get()` and `set()` whatever its channel order, and only `bytes()` shows the difference.

### FbFormat

#### `__construct(int $layout, int $bitOrder = FB_BIT_ORDER_MSB, int $byteOrder = FB_BYTE_ORDER_MSB, int $channelOrder = FB_CHANNELS_RGB, int $scan = FB_SCAN_TOP_DOWN, array $palette = [])`

Describes a byte layout. Immutable.

* `$layout` — one of the `FB_LAYOUT_*` constants.
* `$bitOrder`, `$byteOrder`, `$channelOrder`, `$scan` — the orders above. A bit or byte order the layout doesn't use is accepted and has no effect. A channel order on a layout without channels, or a three-channel order on `FB_LAYOUT_RGBA8888` and a four-channel one on the others, throws `ValueError`.
* `$palette` — one `[int $rgb, bool $inverted, int $code]` per ink, at most 16: the colour as `0xRRGGBB`, whether a planar plane stores the ink as 0, and the word an index layout stores for it. `FB_LAYOUT_PLANAR` needs one; an index layout without one holds grey levels; any other layout refuses one.

Colours map with integer arithmetic on 0..255 channels: Rec. 709 luma for mono, lit at half, and for grey; the nearest palette entry for index and planar layouts, the earlier entry winning a tie; bit packing for RGB.

**Example — a black, white and red e-paper panel**

```php
// Paper is white; ink 1 is black, ink 2 is red.
$epaper = new FbFormat(FB_LAYOUT_PLANAR, palette: [[0x000000, false, 1], [0xFF0000, false, 2]]);

echo $epaper->mapRgba8(20, 20, 20), ' ', $epaper->mapRgba8(250, 10, 10), ' ', $epaper->mapRgba8(255, 255, 255);   // 1 2 0
```

---

#### `layout(): int`, `bitOrder(): int`, `byteOrder(): int`, `channelOrder(): int`, `scan(): int`

What the format was made with.

---

#### `size(int $width, int $height): int`

The bytes a `$width` × `$height` surface takes in this format. Sides run 1..65535.

---

#### `mapRgba8(int $red, int $green, int $blue, int $alpha = 255): int`

Four 0..255 channels to this format's pixel word.

---

#### `unmapRgba8(int $word): int`

A pixel word back to `0xRRGGBBAA`, showing what the layout keeps of a colour.

---

### FbBuffer

#### `__construct(FbFormat $format, int $width, int $height)`

Holds one `$width` × `$height` surface in `$format`, sides 1..65535, allocated through PHP's allocator so it counts against `memory_limit`. It starts blank: every byte zero, and a planar plane whose ink is inverted at 1.

---

#### `format(): FbFormat`, `width(): int`, `height(): int`, `size(): int`

The format it was made with, its sides, and the bytes it holds.

---

#### `granularity(): array`

`[$unitWidth, $unitHeight]`: the smallest block of pixels whose bytes stand alone. `[$width, 8]` for `FB_LAYOUT_MONO_PAGES`, whose bytes are whole columns of a page; `[1, 1]` for every other layout.

---

#### `get(int $x, int $y): int`, `set(int $x, int $y, int $value): void`

Reads or writes one pixel word. `set()` masks the value to what the layout stores.

---

#### `setPixels(array $pixels): ?array`, `setCoordinates(array $coordinates, int $value): ?array`

Writes a list: one `[$x, $y, $value]` per pixel, or one `[$x, $y]` per pixel all in `$value`. Answers `[$x, $y, $width, $height]`, the box the list touched, or `null` for an empty list. A list with any pixel outside the buffer is refused whole.

**Example**

```php
$box = $screen->setPixels([[1, 1, $red], [3, 2, $red]]);   // [1, 1, 3, 2]
```

---

#### `rect(int $x, int $y, int $width, int $height, int $value): void`, `fill(int $value): void`

Fills a rect, which must lie inside the buffer, or the whole buffer. Padding bits at the end of a row are never written.

---

#### `bytes(): string`

The raw store, top row first, exactly as the layout lays it out.

---

#### `layer(int $layer): string`

One plane of a planar buffer, one per palette entry in palette order. Layer 0 of any other layout is the whole store.

---

#### `region(int $x, int $y, int $width, int $height, ?FbFormat $format = null): string`

A rect's bytes, its rows in the format's scan order. With no format the words are copied as stored, in this buffer's own format; with one, every pixel is mapped through RGBA8 into it.

---

#### `rgba8(): string`

The whole buffer as RGBA8 bytes, top-left first.

---

#### `plane(int $value): string`

One bit per pixel, rows padded to a byte, `x` 0 in bit 7: set where the pixel's word equals `$value`. A mask for a panel that takes one colour at a time.

---

#### `blitRgba8(string $rgba8, int $width, int $height, int $x, int $y, int $clipX = 0, int $clipY = 0, ?int $clipWidth = null, ?int $clipHeight = null): ?array`

Maps a `$width` × `$height` RGBA8 image into the buffer with its top-left at `($x, $y)`, words as the format maps them, no blending. Unlike every other call it clips, to the buffer and to the clip rect: a `null` clip side runs to the buffer's edge. Answers the rect written, or `null` when nothing landed. `$rgba8` must be exactly `$width` × `$height` × 4 bytes.

---

#### `copy(FbBuffer $source, int $x, int $y, int $width, int $height): void`

Copies a rect from a buffer of the same size and storage, words as stored.

---

#### `paintSpans(string $spans, int $rgba): ?array`

Paints coverage spans in one colour, `0xRRGGBBAA`. `$spans` holds 7-byte records, little-endian: `y`, `x` and `length` as uint16, `coverage` as uint8. The effective alpha is alpha × coverage. RGB and grey layouts blend source-over; mono, palette and planar layouts write the colour where the effective alpha reaches 128. Every span is checked before any is painted. Answers the spans' bounding box, or `null` for none.

---

#### `paintRgba8(string $rgba8, int $width, int $height, array $inverse, int $x, int $y, int $targetWidth, int $targetHeight, int $opacity = 255, bool $smooth = false, int $row = 0): void`

Paints a straight-alpha RGBA8 image through an inverse placement.

* `$inverse` — `[a, b, c, d, e, f]`, six finite numbers. The centre of each target pixel `(x, y)` maps to the image point `(a·x + c·y + e, b·x + d·y + f)`.
* `$x`, `$y`, `$targetWidth`, `$targetHeight` — the rect of the buffer to visit, which must lie inside it. A pixel whose point falls outside the image is left alone.
* `$opacity` — 0..255; each pixel blends like a span at source alpha × opacity.
* `$smooth` — sample the four source pixels around the point, colours weighed by alpha and edges held, instead of the one under it.
* `$row` — for a buffer that is a window onto a taller surface: the surface row its own row 0 stands for, 0..65535. `y` counts from there, so each window samples exactly as the whole surface would.

---

#### `pointer(): int`

The address of the buffer's bytes, valid while the object lives. Native code can read the pixels in place: pack it into a struct for `ioctl()`, or hand it to FFI. Keep the object alive for as long as anything holds the address.

## Extensions you might use with this one

* [ext-rasterize](https://github.com/php-io-extensions/rasterize): paths, ellipses, rings and one-pixel lines into the exact span bytes `paintSpans()` takes.
* [ext-imgdec](https://github.com/php-io-extensions/imgdec): PNG, JPEG and TIFF into the straight RGBA8 that `blitRgba8()` and `paintRgba8()` take.
* [ext-posi](https://github.com/php-io-extensions/posi): the wire. `posix_write()` puts `bytes()` or `region()` on an SPI or I2C device node, and an `SPI_IOC_MESSAGE` transfer can point its buffer straight at `pointer()`.

## AI-assisted development

This codebase was built with LLM assistance under human direction. Prompts, reviews, and architectural decisions were owned by a professional software engineer — the model accelerates drafting and exploration; it does not replace judgment on APIs, dependency direction, or what ships.

## Agentic Documentation

This repository carries an [Open Knowledge Format](https://github.com/GoogleCloudPlatform/knowledge-catalog/blob/main/okf/SPEC.md) bundle at [`.okf/`](.okf/): Markdown with YAML frontmatter, readable by you and loadable by a coding agent. It records what the README doesn't: how the module is layered, what each call of both classes refuses, which calls move byte runs instead of pixels and the case each one covers, why the build turns off fused multiply-add, the scratch build and both installers, and a log of every change that altered behaviour.

Start at [`.okf/index.md`](.okf/index.md). [`AGENTS.md`](AGENTS.md) is the short rule sheet an agent reads before touching the C, the stubs, or packaging; each rule points into the bundle for the reason behind it.

Point your agent at the repo and it has the same context the maintainer has. Point yourself at it when the README's answer is "it does X" and you want "because".

## Resources

* [`src/core.h`](src/core.h): every layout's word format and byte packing, documented next to the code that implements it.
* [PIE](https://github.com/php/pie): the PHP Foundation's extension installer.
* [`gen_stub.php`](https://github.com/php/php-src/blob/master/build/gen_stub.php): the php-src stub format. [`stubs/`](stubs) holds the API declarations; each `*_arginfo.h` is generated from its stub.
* [Open Knowledge Format](https://github.com/GoogleCloudPlatform/knowledge-catalog/blob/main/okf/SPEC.md): the spec the [`.okf/`](.okf/) bundle follows.

## Running the test suite

The suite is Pest v4 and needs no hardware or image files: every layout packed the way its panel takes it, the colour mapping, refusals, spans, placed images, and every byte-run path held to the per-pixel result over 18 formats. It runs on Linux and macOS. Build the module in the clone, then point `php` at it:

```bash
git clone https://github.com/php-io-extensions/fb.git
cd fb
phpize && ./configure --enable-fb && make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu)"
composer install
php -n -d extension="$PWD/modules/fb.so" vendor/bin/pest
```

```console
Tests:    1 skipped, 975 passed (2784 assertions)
Duration: 0.67s
```

The skipped test is the one that only runs on a 32-bit PHP. `-n` skips your `php.ini`, so the suite tests the clone and not an installed `fb.so`; without it, a PHP that already loads fb warns `Module "fb" is already loaded` and keeps the installed one. A module only loads in the PHP whose `phpize` built it: for Homebrew's ZTS build, run `phpize` and pass `--with-php-config` from `/opt/homebrew/opt/php@8.4-zts/bin`.

## Contributing

Open an issue first for anything beyond a typo. Pull requests change a stub in `stubs/` and the C under `src/` together, regenerate the `*_arginfo.h` files with gen_stub, and add a Pest test for the behaviour. A new byte-run path in `src/core.c` must answer what the per-pixel path answers, which `tests/BulkTest.php` checks over every format. CI must be green on every cell of the matrix. Read [`AGENTS.md`](AGENTS.md) for the rules the code follows.

## Security

ext-fb writes only inside memory it allocated, and hands out that memory's address through `pointer()`. See [SECURITY.md](SECURITY.md) for what it guards against and how to report a vulnerability.

## License

MIT. See [LICENSE](LICENSE).
