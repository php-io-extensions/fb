---
okf_version: "0.2"
---

# php-io-extensions/fb

Opinionated PHP extension: Zephir static class `Fb\Buffer` calls a thin C ABI in `src/fb-api.{c,h}` that stores five framebuffer kinds (full, dirty, epaper, paged, ring) in the host pixel layout. Handles are PHP `int`s. Windows is excluded. No FFI. Version **0.8.0**.

**Prefer** concepts with `status: stable` when present; content is currently `draft` pending Angel’s human verification.

# Architecture

* [Five buffer kinds](architecture/buffers.md) - kinds, layouts, mapper, handles, fixtures

# Fast facts

| | |
|---|---|
| Version | 0.8.0 |
| PHP class | `Fb\Buffer` |
| Zephir | `fb/buffer.zep` |
| C ABI | `src/fb-api.{c,h}` |
| Proof | `scripts/tests/smoke.php`, `scripts/tests/fixtures.php` |
| Downstream | `jovian/fb` owns lifetime and Surface exceptions |
