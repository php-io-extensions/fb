# Agent guidance — php-io-extensions/fb

1. **Read [`.okf/index.md`](.okf/index.md) first** before changing the API, the C, or packaging. Open only the concepts the change touches.
2. **Storage only.** `FbFormat` describes a byte layout; `FbBuffer` holds one surface in it. No damage tracking, pages, frames or drawing primitives beyond what the classes have; no dependency; no framework names. See [`.okf/api/surface.md`](.okf/api/surface.md).
3. **Plain C on the Zend API.** No Zephir, no FFI, no system library. `src/core.{h,c}` is plain C over caller memory with no Zend in it; `src/FbFormat.c` and `src/FbBuffer.c` are the classes over it; `src/fb.c` is the module and its constants. `core.h` documents every word format.
4. **The stubs are the declaration.** Edit `stubs/*.stub.php`, regenerate the matching `*_arginfo.h` with gen_stub, commit both. Never hand-edit arginfo. See [`.okf/runbooks/build.md`](.okf/runbooks/build.md).
5. **Refuse, don't clip.** A coordinate or rect outside the buffer is a `ValueError`, and a list is checked whole before anything is written. `blitRgba8()` is the one call that clips, by contract.
6. **Every size fits the platform.** Sides run 1..65535, and `(width + 8) × height × 4` must fit a `zend_long` before anything is allocated, so no byte count wraps on a 32-bit PHP. A new allocation sizes from sides already inside a buffer, or checks its own product in 64-bit arithmetic.
7. **Byte runs answer what pixels answer.** A bulk path in `core.c` must produce the per-pixel result byte for byte; `tests/BulkTest.php` holds every path to it over 18 formats. Padding bits are never written. See [`.okf/api/surface.md`](.okf/api/surface.md).
8. **`-ffp-contract=off` stays in `config.m4`.** `paintRgba8()` maps pixel centres in doubles, and a fused multiply-add moves a sample point. Check with `objdump -d src/.libs/core.o | grep -c fmadd`: 0.
9. **NTS and ZTS.** No module globals. Every change builds with `-Wall -Wextra` without a warning from `src/` and passes Pest under NTS and ZTS on macOS and Linux.
10. **Tests are Pest v4 and make their own inputs.** No fixture files, no hardware. Expected bytes come from the layouts' datasheet packing.
11. **Durable facts go in `.okf`.** Update the matching concept, bump its `generated.at`, append `.okf/log.md`. The bundle documents the package, never a session.
12. **Version**: `PHP_FB_VERSION` moves only past a published tag; tags drive Packagist.
