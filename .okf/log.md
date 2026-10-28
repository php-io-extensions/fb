# Log

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
