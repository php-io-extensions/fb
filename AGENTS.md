# Agent guidance — php-io-extensions/fb

1. **Read [`.okf/index.md`](.okf/index.md) first** before changing architecture, API, or packaging.
2. Open only the concept files you need; prefer `status: stable` when present (content starts `draft` until Angel verifies).
3. This package is **Linux + macOS** framebuffer bytes in C — Windows excluded. No FFI. Thin C ABI in `src/fb-api.{c,h}`.
4. Public PHP API: flat namespace `Fb\Buffer` — static methods only. Zephir source: `fb/buffer.zep`. Every method is one C call. Do not invent methods.
5. **The ext is opinionated by decision.** Its whole job is these five buffer kinds (full, dirty, epaper, paged, ring). Do not grow a general drawing or packing API here. Colour policy and Surface exceptions live in `jovian/fb`.
6. **Byte layouts** are the Surface spec packings table (also in `.okf/architecture/buffers.md`). Implement from that table and the fixtures — never from `venusian/surface/src/Surface/Framebuffers` (no-bleedover).
7. **Fixtures are the truth.** `venusian/surface/tests/Framebuffers/fixtures/*.json` (27 files). A miss is a DERIVE bug until proven otherwise. Do not edit fixture JSON to pass this ext. `scripts/tests/smoke.php` and `scripts/tests/fixtures.php` are the gates: `FB_SMOKE_OK`, `FB_FIXTURES_OK 27`.
8. **Constants** (kind / format / bit-order ints) live **outside** the extension — `jovian/fb` enums mirror the C `#define`s. No PHP class constants.
9. **`ext/` is the PIE/`phpize` input.** Commit the generated C (`fb.c`, `fb/buffer.zep.c`, `src/fb-api.c`, kernel, `config.m4`, `php_fb.h`). Do **not** commit phpize junk (`Makefile`, `configure`, `modules/*.so`, `autom4te.cache`, `*.lo`, `*.dep`). After a local build, delete those artifacts so the folder is commit-ready. Do **not** treat installer side effects as a license to rewrite release C casually.
10. Build/install for local use: `install-macos.sh` or `install-debian-trixie.sh`. On an SBC, run those on the board (via `fnk`); do not execute installer/composer against a mount.
11. Downstream: `jovian/fb` is the typed projection and the `native` Surface driver. It owns handle lifetime. This ext never throws: out-of-range `get` → 0, `set` ignored.
12. When you learn a durable package fact, **update the matching `.okf` concept**, bump `generated.at`, and append `.okf/log.md`. New/changed concepts stay `status: draft` until a human verifies them.
13. Keep the OKF bundle at package root only — never nest `.okf` under `fb/` or `src/`.
