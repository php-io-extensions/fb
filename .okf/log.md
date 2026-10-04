# Log

## 2026-10-03

* Bundle created with ext-fb 0.10.0, a plain-C rewrite: [surface](api/surface.md) (`FbFormat`, `FbBuffer`, 23 constants), [build](runbooks/build.md).
* Bulk paths in the pixel core: byte runs for whole-byte layouts and whole rows, byte reorder between 32-bit orders; `tests/BulkTest.php`. [surface](api/surface.md)
* `FbBuffer::paintSpans()`: coverage spans blended or thresholded in C, version unchanged; `src/` builds warning-free under `-Wall -Wextra`. [surface](api/surface.md)
* `FbBuffer::paintRgba8()`: an RGBA8 image placed through an inverse affine, nearest or smooth, blended; built `-ffp-contract=off`; version unchanged. [surface](api/surface.md), [build](runbooks/build.md)
* `paintRgba8()` gains `row`: the surface row the buffer's row 0 stands for, so a window onto a taller surface samples exactly as the whole would. [surface](api/surface.md)
