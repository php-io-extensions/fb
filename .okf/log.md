# Log

## 2026-11-07

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
