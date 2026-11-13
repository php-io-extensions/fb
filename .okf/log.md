# Log

## 2026-11-13

* `FbBuffer::over()`: a buffer over bytes another object owns, for painting into another extension's memory in place. The buffer holds a borrowed flag and the owner; `free_obj` leaves borrowed bytes alone. `tests/OverTest.php`, also run under guard malloc. [surface](api/surface.md)

## 2026-11-07

* Glass (Surface drawing-path spec, G4): `FbBuffer::kawaseDown()`, `kawaseUp()` (a dual-filter pyramid over RGBA8888 levels, integer: down (4·C + S + 16) >> 5, up the 8-tap tent in quarter pixels / 192) and `paintGlass()` (one pane per call over its rounded box: SDF, refraction along the normal, bilinear sample of the level, frost, saturation, tint, light, rim light, integer grain hash, anti-aliased edge). `core.h` holds the arithmetic; `tests/GlassTest.php` holds it to a PHP reference. Version unchanged.
* Span sink (`src/sink.h`): the exported `phpfb_span_sink` (version 1: `size()`, `paint_spans()`), for ext-rasterize to paint spans into an `FbBuffer` from C without linking to ext-fb; `paintSpans()` and the sink share one checker and painter. Version unchanged.
* Faster span painting, no API change (Surface drawing-path spec, C2a). Opaque runs, and every `fb_fill_rect()` row, store the first pixel and copy its bytes along, doubling, in every whole-byte layout (`fb_fill_row()`), instead of a `fb_set()` switch per pixel. Translucent runs under 32 pixels blend in place in RGBA8888, RGB888 and 8-bit grey (`fb_blend_short()`: the table expression, no get, unmap, map or set). `paintSpans()`, `paintMasks()` and `fill`/`rect` all gain. Byte-identical: `tests/Fixtures/blend.json` and the per-pixel reference, now with runs past the table threshold (`tests/BulkTest.php`). RGBA8 300×200 anti-aliased rounded rect: opaque 128.7 → 13.8 µs, translucent 77.2 → 74.1 µs. Tried and dropped: arithmetic or 64-bit four-lane blending for long RGBA8888 runs (84 and 89 µs, slower than the tables). Version unchanged.
* `FbBuffer::paintMasks(masks, rgba, clipX, clipY, clipWidth, clipHeight)` paints a list of coverage masks (`[coverage, width, left, top]`, rows top-down, placed within ±2^30 and cut to the clip) in one call, in order, each pixel of coverage above 0 as a span of that coverage: `fb_paint_masks()` and `fb_paint_spans()` share one run painter in `core.c`. For Surface's text: one call per run of glyphs, from masks cached once. Byte-identical to `paintSpans()` of the same runs over 18 formats (`tests/BulkTest.php`). Version unchanged.

* `FbBuffer::filter()` knows two more steps, `field` (an image from the pixel position: the windowed distance and direction of a rounded box, an inside mask, or a ramp) and `table` (a 256-byte lookup for each of the straight channels). `src/filter.c`; byte-identical to Surface's `NativeFilters` (`FilterTest.php` there). Version unchanged.

* `FbBuffer::filter()` takes an eighth argument, `premultiplied = false`: the region's pixels are premultiplied RGBA8 already (what a layer drawn over nothing leaves) and are the source as they are; the result is straight either way. A list of no steps is allowed and only straightens. Version unchanged.

* **Version stays 0.11.1**: `FbBuffer::filter(steps, maps, x, y, width, height, row = 0)` runs a filter chain over a region (`src/filter.c`): blur (gaussian by SVG's three boxes, or a kernel below a deviation of 2; spiral), colour matrix, flood, composite, blend, noise (SVG turbulence and fractal, or an integer hash) and displacement with dispersion bands. Pixels in and out are straight RGBA8; the steps work on premultiplied values. Steps are checked before any pixel is read. Byte-identical to Surface's `NativeFilters` (`tests/Framebuffers/FilterTest.php` there: 16 chains, whole surface and a region).

* **Same day, version stays 0.11.1**: `FbBuffer::paintGradient(spans, ramp, radial, spread, inverse, row = 0)` paints a gradient through spans (ramp of 256 RGBA8 colours; t = u or hypot(u, v); spread 0 pad, 1 repeat, 2 reflect; entry floor(t × 255 + ½); u and v worked out at a span's first pixel and stepped along it; alpha = ramp alpha × coverage, blended like a span). Surface's `Framebuffer::paintGradient()` calls it, so VelvetGE holds no pixel loop. Byte-identical to Surface's native store (`tests/Framebuffers/PaintGradientTest.php` there).

* **Version**: 0.11.1 in `PHP_FB_VERSION` (`php_fb.h`) and the version test. `paintRgba8()` takes `$tint`, `$window` and `$coverage` (arguments 12 to 14): the tint (alpha too), then the opacity, then the coverage of the mask span that holds the pixel, each (x × y + 127) / 255; the window crops while smooth sampling still reads the whole image. A mask's spans must be inside the buffer, rows ascending, x ascending without overlap, or the call throws `ValueError` before painting. Byte-identical to Surface's native store.

## 2026-11-05

* `paintSpans()` blends spans of 32 pixels or more by table (`fb_blend_table()`: the per-pixel expression for all 256 destination levels, one 1 KB slot rebuilt when the alpha changes); `paintRgba8()` hoists the row pointer and the inverse's y products and blends RGBA8888 on its bytes. No API change; byte-identical to 0.11.0 by `tests/Fixtures/blend.json` (1636 scenarios, `capture.php`). 1800×1280 RGBA8: a full-surface translucent span 20.9 → 3.7 ms, a full-size `paintRgba8` 24.6 → 11.0 ms. A running sum for the sample point moves the last bit and was not taken.
* Tests may hold a captured fixture (`tests/Fixtures/`) where today's build is the reference.

## 2026-10-28

* **Version**: 0.11.0 in `PHP_FB_VERSION` (`php_fb.h`); `branch-alias` `dev-main` is `0.11.x-dev`. Promoted with the Venusian 0.11 ecosystem. No API or behaviour change. Built and loaded on Homebrew PHP 8.4 NTS and ZTS, the Pi on 8.4 ZTS and an x86_64 8.4 NTS box: 975 passed, 1 skipped on each.

## 2026-10-03

* Bundle created with ext-fb 0.10.0, a plain-C rewrite: [surface](api/surface.md) (`FbFormat`, `FbBuffer`, 23 constants), [build](runbooks/build.md).
* Bulk paths in the pixel core: byte runs for whole-byte layouts and whole rows, byte reorder between 32-bit orders; `tests/BulkTest.php`. [surface](api/surface.md)
* `FbBuffer::paintSpans()`: coverage spans blended or thresholded in C, version unchanged; `src/` builds warning-free under `-Wall -Wextra`. [surface](api/surface.md)
* `FbBuffer::paintRgba8()`: an RGBA8 image placed through an inverse affine, nearest or smooth, blended; built `-ffp-contract=off`; version unchanged. [surface](api/surface.md), [build](runbooks/build.md)
* `paintRgba8()` gains `row`: the surface row the buffer's row 0 stands for, so a window onto a taller surface samples exactly as the whole would. [surface](api/surface.md)

## 2026-10-13

* `FbBuffer::__construct()` and `FbFormat::size()` refuse sides a 32-bit PHP cannot address, where the byte count used to wrap `size_t`; `blitRgba8()` and `paintRgba8()` check their image length in 64-bit arithmetic. No change on 64-bit. [surface](api/surface.md)

## 2026-10-13

* Version 0.10.5: `PHP_FB_VERSION` and the version test move from 0.10.0. README rewritten to the php-io-extensions template.
