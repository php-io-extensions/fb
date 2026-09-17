# fb

C-backed framebuffer storage for PHP. One class, `Fb\Buffer`: static methods, integer handles, bytes already in the panel’s host layout.

Venusian Surface’s CPU path uses this so renderers are not walking PHP arrays. Call `Fb\Buffer` yourself only when you are installing or proving the extension.

## Features

- **Five buffer kinds** — full, dirty (merged damage), epaper, paged (one window), ring (front/back frames)
- **Host-format store** — mono, packed index, RGB444/565/666/888, RGBA8888, planar; the bytes are the panel layout
- **One C call per method** — no FFI
- **Never throws** — out-of-range `get` returns `0`, `set` is ignored; `create` returns `0` when it refuses

## Requirements

- PHP 8.0 or newer (the house stack is 8.4)
- Linux or macOS. Windows is excluded.
- From-source install: a C toolchain, [Zephir](https://github.com/zephir-lang/zephir), and `phpize` / `php-dev`

## Quick start

Install, then write an 8×2 mono buffer. This is the same sequence as `scripts/tests/smoke.php`.

```bash
./install-macos.sh          # macOS
# ./install-debian-trixie.sh  # Debian Trixie / Raspberry Pi OS
```

```php
<?php

$h = Fb\Buffer::create(
    kind: 0,            // full
    pixelFormat: 1,     // mono_horizontal
    bitDepth: 1,
    bitOrder: 0,        // msb_first
    endianness: -1,     // default
    pageAxis: -1,       // default
    scan: 0,            // top_to_bottom
    palette: [],
    width: 8,
    height: 2,
    extra: 0,
);

if ($h === 0) {
    fwrite(STDERR, "create refused\n");
    exit(1);
}

Fb\Buffer::set($h, 0, 0, 1);
Fb\Buffer::set($h, 7, 0, 1);
Fb\Buffer::setSegment($h, 2, 1, 4, 1, 1);

echo bin2hex(Fb\Buffer::bytes($h)), "\n";  // 813c
Fb\Buffer::free($h);
```

Confirm the module:

```bash
php --ri fb
php scripts/tests/smoke.php       # FB_SMOKE_OK
```

## Installation

### macOS

```bash
cd fb
./install-macos.sh
```

On Laravel Herd, use this installer rather than PIE.

### Linux (Debian Trixie / Raspberry Pi OS)

```bash
cd fb
./install-debian-trixie.sh
```

Needs `php-dev` / `phpize` and a C compiler. The installer writes `fb.so` and a `30-fb.ini`.

### PIE

```bash
pie install php-io-extensions/fb
```

PIE builds from the committed `ext/` tree. If it does not enable the module, add:

```ini
extension=fb
```

Verify: `php -m` lists `fb`, and `class_exists('Fb\\Buffer')` is true.

## Usage

`create` answers a handle (`int`). `0` means refusal. `free(0)` is a no-op. You own the handle until `free`.

### Writes and reads

```php
<?php

$h = Fb\Buffer::create(0, 1, 1, 0, -1, -1, 0, [], 8, 2, 0);
Fb\Buffer::set($h, 0, 0, 1);
Fb\Buffer::fill($h, 0);
Fb\Buffer::setPixels($h, [[0, 0, 1], [7, 0, 1]]);

echo Fb\Buffer::get($h, 0, 0), "\n";           // 1
echo Fb\Buffer::get($h, 1, 0), "\n";           // 0 — also what you get for out-of-range
echo bin2hex(Fb\Buffer::toRgba8($h)), "\n";    // RGBA8, always top-down

Fb\Buffer::free($h);
```

### Dirty damage

```php
<?php

$h = Fb\Buffer::create(1, 2, 16, -1, -1, -1, 0, [], 16, 16, 0);
Fb\Buffer::beginEpoch($h);
Fb\Buffer::set($h, 1, 1, 1);
Fb\Buffer::set($h, 2, 2, 1);

print_r(Fb\Buffer::damage($h));
// [[1, 1, 2, 2]]  — touching rects merge; snapped to granularity

Fb\Buffer::free($h);
```

### When create refuses

```php
<?php

// ring needs extra >= 2
$h = Fb\Buffer::create(4, 1, 1, 0, -1, -1, 0, [], 8, 1, 1);
var_dump($h);  // int(0)
```

`create` also returns `0` for an unsupported format, epaper that is not planar / mono-horizontal / paletted index, paged row counts that the layout rejects, or width or height below 1.

## `create` arguments

Kinds (`$kind`) and `$extra`:

| Kind | Int | Extra |
|------|-----|--------|
| full | 0 | unused |
| dirty | 1 | unused |
| epaper | 2 | unused |
| paged | 3 | page rows |
| ring | 4 | frame count (≥ 2) |

Format ints. Pass `-1` to apply the default for that field.

| Field | Values | Default when `-1` |
|-------|--------|-------------------|
| pixel format | 0 vertical-page, 1 horizontal, 2 row-major, 3 planar | required (no default) |
| bit order | 0 MSB, 1 LSB | MSB; LSB for vertical-page |
| endianness | 0 LSB, 1 MSB | MSB for RGB565 |
| page axis | 0 horizontal, 1 vertical | vertical (vertical-page only) |
| scan | 0 top-to-bottom, 1 bottom-to-top | top-to-bottom |

`$palette` is a list of `[rgb, inverted, code]` (`rgb` is `0xRRGGBB`). Empty means no palette.

The store is always top-down. `$scan` only reverses rows on `region()` / transcode output. `toRgba8()` is always top-down.

Full method list: [`fb/buffer.zep`](fb/buffer.zep). Layouts and mapper: [`.okf/architecture/buffers.md`](.okf/architecture/buffers.md).

## What this is not

- Not a drawing API. No shapes, text, or GPU.
- Not the Surface `Framebuffer` contract. That wrapper lives above this extension.
- Does not throw. Range checks and Surface exceptions belong above this extension.
- Does not run on Windows.

## Tests

```bash
php scripts/tests/smoke.php
php scripts/tests/fixtures.php    # FB_FIXTURES_OK 27
```

`FB_FIXTURES` points at `venusian/surface/tests/Framebuffers/fixtures` (27 JSON files). The default path is that directory relative to this repo. Those fixtures are the contract. Do not edit them to make this extension pass.

## License

[MIT](LICENSE)
