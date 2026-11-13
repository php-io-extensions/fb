---
type: API
title: Surface
description: Constants, FbFormat, FbBuffer: every method, what it refuses.
resource: stubs/
tags: [fb, pixels, c]
status: draft
generated: { by: claude-fable/5.2, at: 2026-11-13T09:00:00Z }
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

`__construct(FbFormat, width, height)`, sides 1..65535, and `(width + 8) × height × 4` at most `ZEND_LONG_MAX` (that product bounds every layout's bytes and its RGBA8 form; only a 32-bit PHP reaches it, from about 23166² up, and `FbFormat::size()` refuses the same sides). Starts as `fill(0)`: zero bytes, inverted planar planes at 1.

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
| `filter(steps, maps, x, y, width, height, row = 0, premultiplied = false)` | Replaces a region (inside the buffer) with a filter chain run over its pixels: straight RGBA8 in and out, the steps premultiplied (with `premultiplied` the pixels in are premultiplied already and taken as they are; no steps only straightens). A step is a list, its kind first: `blur`, `matrix`, `flood`, `composite`, `blend`, `noise`, `displace`, `field`, `table`; it reads `source` (the region's pixels), an earlier step's named result, or the previous result; `maps` holds `[width, height, RGBA8]` displacement maps. Noise is a function of the surface position, y counted from `row`. `field` writes an image from the position: `['field', kind, x0, y0, x1, y1, r0, r1, r2, r3, from, to, result]`, kind 0 the windowed distance to a rounded box in red and its outward direction in green and blue, 1 a mask of the inside, 2 a ramp down the surface (box ignored); `table` is `['table', in, red, green, blue, alpha, result]`, each a 256-byte string or null: the straight channel goes through it, then the pixel is premultiplied by the new alpha. Every step is checked first; a bad one throws `ValueError` and nothing is read |
| `paintSpans(spans, rgba)` | 7-byte spans (y, x, length uint16; coverage uint8) in `0xRRGGBBAA`; all checked first; effective alpha `(alpha × coverage + 127) / 255`; RGB and palette-less grey blend source-over through the mapper, mono, index-with-palette and planar write where it reaches 128; returns the spans' bounding box or null |
| `paintMasks(masks, rgba, clipX, clipY, clipWidth, clipHeight)` | coverage masks `[string coverage, int width, int left, int top]` (rows top-down, whole rows of 1..65535 bytes, placed within ±2^30), each cut to the clip (non-empty, inside the buffer), painted in order; every pixel of coverage above 0 painted as `paintSpans` paints a span of that coverage; all checked first; returns the box of the pixels with coverage or null |
| `paintGradient(spans, ramp, radial, spread, inverse, row = 0)` | A gradient through 7-byte spans: `ramp` is 256 RGBA8 colours; a pixel's centre goes through `inverse` `[a, b, c, d, e, f]` (y counted from `row`) to `(u, v)`, `t = u` or `hypot(u, v)` when `radial`; `spread` 0 pad, 1 repeat, 2 reflect folds t into 0..1; the colour is `ramp[floor(t × 255 + ½)]`, blended like a span at `(ramp alpha × coverage + 127) / 255`; `(u, v)` is worked out at a span's first pixel and stepped along it; returns the spans' bounding box or null |
| `paintRgba8(rgba8, w, h, inverse, x, y, tw, th, opacity = 255, smooth = false, row = 0)` | RGBA8 image through an inverse placement `[a, b, c, d, e, f]`: each pixel of the target rect (inside the buffer) maps its centre to `(a·x + c·y + e, b·x + d·y + f)`, y counted from `row` (the row of a larger surface this buffer's row 0 stands for); inside the image it takes the pixel under it, or with `smooth` the four around it (1/256 weights, colours weighed by alpha, edges held); blended like a span at `(source alpha × opacity + 127) / 255` |
| `FbBuffer::over(format, width, height, address, owner)` | static: a buffer over `size()` packed bytes at `address` that `owner` owns; nothing allocated, copied or blanked; keeps `owner` alive and never frees the bytes; address unchecked beyond non-zero; the span sink paints into it as into any buffer |
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
