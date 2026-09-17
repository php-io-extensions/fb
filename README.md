# ext-fb 0.8.0

Five framebuffer kinds in C. PHP sees one class: `Fb\Buffer`. Static methods, int handles. No throw. Bad `get` answers 0. Bad `set` dropped. `jovian/fb` owns handle lifetime and Surface exceptions.

No FFI. Linux + macOS. Windows out.

## Install

```bash
# macOS
./install-macos.sh

# Debian Trixie / Raspberry Pi OS
./install-debian-trixie.sh
```

Proof after install:

```bash
php scripts/tests/smoke.php          # FB_SMOKE_OK
php scripts/tests/fixtures.php       # FB_FIXTURES_OK 27
```

`FB_FIXTURES` names the Surface JSON dir. Default is `../../venusian/surface/tests/Framebuffers/fixtures` from this repo root.

## `Fb\Buffer`

| Method | Signature | Semantics |
|---|---|---|
| `create` | `(int $kind, int $pixel_format, int $bit_depth, int $bit_order, int $endianness, int $page_axis, int $scan, array $palette, int $width, int $height, int $extra): int` | mint handle. 0 on refuse. `$kind` 0 full, 1 dirty, 2 epaper, 3 paged (`$extra` = page rows), 4 ring (`$extra` = frames). `-1` = apply defaults. `$palette` = list of `[rgb, inverted, code]`. Refuses unsupported format, epaper rules, paged rows rules, ring < 2. |
| `free` | `(int $h): void` | release; 0 is no-op |
| `width` / `height` | `(int $h): int` | virtual surface |
| `size` | `(int $h): int` | byte length of `bytes()` |
| `get` | `(int $h, int $x, int $y): int` | host word; paged outside page → 0; ring reads front |
| `set` | `(int $h, int $x, int $y, int $v): void` | paged outside page → drop; ring writes back |
| `setPixels` | `(int $h, array $pixels): void` | list of `[x, y, v]` |
| `setSegment` | `(int $h, int $x, int $y, int $w, int $hh, int $v): void` | clipped to surface (and page) |
| `fill` | `(int $h, int $v): void` | real pixels only |
| `bytes` | `(int $h): string` | store, top-down: whole / current page (real rows) / front frame |
| `region` | `(int $h, int $x, int $y, int $w, int $hh, int $scan): string` | sub-rect in host layout; `$scan` 1 reverses rows |
| `layer` | `(int $h, int $layer): string` | one planar plane; `-1` = all |
| `pointer` | `(int $h): int` | address of store bytes |
| `toRgba8` | `(int $h): string` | RGBA8 top-down of store |
| `blitRgba8` | `(int $h, string $rgba8, int $sw, int $sh, int $ox, int $oy): void` | write through mapper, clip |
| `transcode` | `(int $h, int $pixel_format, int $bit_depth, int $bit_order, int $endianness, int $page_axis, int $scan, array $palette, int $x, int $y, int $w, int $hh): string` | sub-rect re-packed through RGBA |
| `granularity` | `(int $h): array` | `[unit_w, unit_h]` |
| `beginEpoch` | `(int $h): void` | dirty only |
| `damage` | `(int $h): array` | snapped merged `[x, y, w, h]` list; `[]` if not dirty |
| `setPage` / `page` / `pages` | `(int $h, int $p): void` / `(int $h): int` / `(int $h): int` | paged only; `setPage` zeroes window |
| `present` | `(int $h): void` | ring only |
| `frames` | `(int $h): int` | ring only, else 0 |

## Enum ints

Task 6 (`jovian/fb`) mirrors these. `-1` means apply defaults.

| Field | Ints |
|---|---|
| pixel_format | 0 `mono_vertical_page`, 1 `mono_horizontal`, 2 `row_major`, 3 `planar` |
| bit_order | 0 `msb_first`, 1 `lsb_first` |
| endianness | 0 `lsb`, 1 `msb` |
| page_axis | 0 `horizontal`, 1 `vertical` |
| scan | 0 `top_to_bottom`, 1 `bottom_to_top` |

Defaults: mono horizontal + planar bit order MSB; vertical-page bit order LSB + page axis vertical; packed index MSB; B16 endianness MSB.

## Rules

- Ext never throws. `jovian/fb` guards range and throws Surface exceptions.
- Store is host format. `toRgba8()` always top-down. Scan reverses on output only.
- Padding bits stay 0. Initial state is `fill(0)` — inverted planar planes start at 1.
- Fixtures in `venusian/surface/tests/Framebuffers/fixtures` are the truth. Do not edit them to pass this ext.
- `ext/` is the PIE/`phpize` C tree. After a local build, delete phpize junk (`Makefile`, `configure`, `modules/*.so`). Do not leave build artifacts in `ext/`.
