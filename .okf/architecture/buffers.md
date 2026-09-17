---
type: Architecture
title: Five framebuffer kinds
description: ext-fb stores five buffer kinds in C behind Fb\Buffer handles
tags: [fb, framebuffer, zephir, packing]
status: draft
generated: { by: cursor-grok-4.6, at: "2026-09-17T23:30:00Z" }
sources:
  - id: plan-t5
    resource: venusian/surface/docs/superpowers/plans/2026-09-17-cpu-rendering.md
    title: CPU rendering plan Task 5
  - id: spec-s2
    resource: venusian/surface/docs/superpowers/specs/2026-09-17-cpu-rendering-design.md
    title: CPU rendering spec §2 packings
  - id: fixtures
    resource: venusian/surface/tests/Framebuffers/fixtures
    title: 27 golden framebuffer fixtures
---

# Overview

`ext-fb` is opinionated: it exists to hold these five kinds in the panel's own byte format. PHP talks only to `Fb\Buffer`. `jovian/fb` is the typed projection and the Surface `native` driver.[^plan-t5]

# Kinds

`$kind` on `Fb\Buffer::create`:

| Int | Kind | Extra | Notes |
|---|---|---|---|
| 0 | full | unused | whole surface |
| 1 | dirty | unused | damage list, merge on touch, collapse past 16 |
| 2 | epaper | unused | planar + palette, or mono horizontal B1, or packed index + palette |
| 3 | paged | page rows | one window; writes outside the page dropped; `setPage` zeroes |
| 4 | ring | frames ≥ 2 | writes go to back; `present` advances; reads from front |

`create` returns 0 on unsupported format, epaper refuse, paged rows (must be ≥ 1; vertical-page rows multiple of 8), ring < 2.

# Layouts

Chosen at create from `(pixel_format, bit_depth, page_axis)`:

| Layout | Int | Selected by | Bytes | Word |
|---|---|---|---|---|
| mono horizontal | 0 | `mono_horizontal` B1; or `mono_vertical_page` B1 with page_axis horizontal | `ceil(w/8) * h` | 0/1 |
| mono vertical page | 1 | `mono_vertical_page` B1, page_axis vertical | `w * ceil(h/8)` | 0/1 |
| packed index | 2 | `row_major` B2/B4/B8 | `ceil(w*bits/8) * h` | code / grey |
| RGB444 | 3 | `row_major` B12 | `((w+1)/2)*3 * h` | `0xRGB` |
| RGB565 | 4 | `row_major` B16 | `w*h*2` | 16-bit |
| RGB666 | 5 | `row_major` B18 | `w*h*3` | RGB888, low 2 bits zero |
| RGB888 | 6 | `row_major` B24 | `w*h*3` | `0xRRGGBB` |
| RGBA8888 | 7 | `row_major` B32 | `w*h*4` | `0xRRGGBBAA` |
| planar | 8 | `planar` B1 + palette | `ceil(w/8)*h * planes` | channel mask |

Padding bits stay 0. Initial state is `fill(0)` — inverted planar planes start at 1. Store is always top-down; scan reverses on output only.

# Mapper

Rec. 709 luma `0.2126r + 0.7152g + 0.0722b` on 0..1 floats.

- mono: luma ≥ 0.5 → 1; unmap 1 → white, 0 → black
- grey (B8, no palette): `round(luma*255)`
- RGB: quantise then B12/B16/B18/B24/B32 pack; unmap divides by channel max (15, 31/63/31, 252, 255)
- planar: nearest squared RGB over `{white=0} ∪ {palette k → 1<<k}`, ties to the earlier entry; unmap lowest set bit, 0 → white
- packed index: nearest palette entry → code; unknown → white

RGBA8 out is `round(c*255)` per channel, alpha 255 except B32.

# Handles

`create` answers an `int` (the `fb_buffer *`). `free(0)` is a no-op. The ext never throws. `jovian/fb` owns lifetime.

# Fixture contract

`scripts/tests/fixtures.php` runs every JSON in `venusian/surface/tests/Framebuffers/fixtures` (27). Gate: `FB_FIXTURES_OK 27`. Smoke (`813c` from fixture 01 first step): `FB_SMOKE_OK`. The runner cannot check `page_region` / `preserves` / `pointer_zero` — those belong to Task 6.

Do not read `Surface\Framebuffers\Php` when changing this package.

[^plan-t5]: Task 5 listing in the CPU rendering plan
