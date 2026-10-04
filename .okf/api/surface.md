---
type: API
title: Surface
description: Constants, FbFormat, FbBuffer: every method, what it refuses.
resource: stubs/
tags: [fb, pixels, c]
status: draft
generated: { by: claude-opus/5.5, at: 2026-10-03T23:06:56Z }
sources:
  - id: stubs
    resource: stubs/
    title: fb.stub.php, FbFormat.stub.php, FbBuffer.stub.php
  - id: core
    resource: src/core.h
    title: The pixel core
---

# Overview

Storage only. No kinds, damage, pages, frames: callers build those. No dependency, no framework names.[^stubs]

Layers: `src/core.{h,c}` plain C over caller memory (layouts, mapper, loops; no Zend). `src/FbFormat.c`, `src/FbBuffer.c` Zend classes over it. `src/fb.c` module + constants.[^core]

# FbFormat

Immutable. `__construct(layout, bitOrder, byteOrder, channelOrder, scan, palette)`. Getters `layout()`, `bitOrder()`, `byteOrder()`, `channelOrder()`, `scan()`; `size(w, h)` bytes; `mapRgba8(r, g, b, a = 255)` → word; `unmapRgba8(word)` → `0xRRGGBBAA`.

Refused (`ValueError`): unknown layout/order/scan; three-channel order on 32-bit and the reverse; channel order on a non-RGB layout; planar without palette; palette on a non-index, non-planar layout; palette code past the layout's bits; more than 16 inks; malformed ink.

# FbBuffer

`__construct(FbFormat, width, height)`, sides 1..65535. Starts as `fill(0)`: zero bytes, inverted planar planes at 1.

| Call | Does |
|---|---|
| `get`, `set` | one pixel word; a word is masked to what the layout stores |
| `setPixels([[x, y, v]…])`, `setCoordinates([[x, y]…], v)` | whole list checked first; returns bounding box `[x, y, w, h]` or null |
| `rect(x, y, w, h, v)`, `fill(v)` | rect must be inside; padding bits untouched |
| `bytes()`, `layer(n)` | raw store; one plane of a planar buffer |
| `region(x, y, w, h, ?FbFormat)` | null: words as stored, own scan order; a format: every pixel through RGBA8, that format's scan order |
| `rgba8()` | RGBA8, top-left first |
| `blitRgba8(rgba8, w, h, x, y, clipX, clipY, ?clipW, ?clipH)` | maps pixels in, clipped to buffer and clip; returns what was written or null |
| `copy(FbBuffer, x, y, w, h)` | same size + storage; words as stored |
| `plane(v)` | 1 bpp, x0 in bit 7, set where word == v |
| `paintSpans(spans, rgba)` | 7-byte spans (y, x, length uint16; coverage uint8) in `0xRRGGBBAA`; all checked first; effective alpha `(alpha × coverage + 127) / 255`; RGB and palette-less grey blend source-over through the mapper, mono, index-with-palette and planar write where it reaches 128; returns the spans' bounding box or null |
| `pointer()`, `granularity()` | address of the bytes; `[w, 8]` for mono pages, `[1, 1]` otherwise |

Both classes: final, no clone, no serialize, second `__construct` is `Error`.

# Bulk paths

`rect`, `fill`, `copy`, `region`, `rgba8`, `blitRgba8` move byte runs where the layout allows. Result identical to the per-pixel path; `tests/BulkTest.php` holds each call to it over 18 formats.[^core]

| Case | Path |
|---|---|
| whole-byte pixels (`INDEX8`, `RGB565`, `RGB666`, `RGB888`, `RGBA8888`), any rect | fill: first row set, rest copied from it; copy and region as stored: one row copy each |
| whole rows of a row-packed layout (all but mono pages), planar per plane | same |
| mono pages, rect on page boundaries | fill: one `memset` per page |
| 32-bit to 32-bit region, `rgba8()` and `blitRgba8()` on a 32-bit buffer | bytes reordered, no mapper |
| whole-byte to whole-byte region, other layouts | row walk through the mapper |
| bit-packed partial rect, bit-packed conversion | per pixel |

Padding bits are never written, so every row carries the same ones and a whole-row copy leaves them as they were.

[^stubs]: The stubs are the declaration of record; arginfo is generated from them.
[^core]: `core.h` documents every word format.
