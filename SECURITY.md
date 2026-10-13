# Security Policy

## Supported versions

ext-fb is pre-1.0. No 0.x release receives security fixes or advisories; fixes land in the
next release line. Security support starts with 1.0.

| Version | Security fixes |
|---------|----------------|
| < 1.0   | No             |

## Reporting a vulnerability

Please don't open a public issue for a security problem.

Report it privately through GitHub: the **Report a vulnerability** button on the
[php-io-extensions/fb](https://github.com/php-io-extensions/fb) repository's **Security** tab.
If that isn't available, email **info@projectsaturnstudios.com**.

Include what you found, the affected version, your platform, whether your PHP is 32-bit or
64-bit and NTS or ZTS, the format and sides of the buffer involved, and steps to reproduce.
Reports are read and weighed for the release line in development; before 1.0 there is no
response-time commitment.

## Security model

ext-fb is pixel storage in plain C: it opens no files, makes no network requests and links no
library. The memory it touches is the buffers it allocated and the strings PHP hands it.

- **Sizes.** A buffer's sides run 1..65535, and `(width + 8) × height × 4`, a bound on the
  bytes of every layout and of its RGBA8 form, must fit a `zend_long` before anything is
  allocated. On a 64-bit PHP no side pair reaches that bound; on a 32-bit PHP it refuses the
  sides whose byte count would wrap `size_t`. Buffers come from PHP's allocator, so
  `memory_limit` bounds a process holding many of them.
- **Writes.** Every coordinate and rect is checked against the buffer before a byte moves, and
  a list of pixels or spans is checked whole before any of it is written. `blitRgba8()` clips
  to the buffer instead, by contract. Padding bits at the end of a row are never written.
- **Reads.** An RGBA8 image passed to `blitRgba8()` or `paintRgba8()` must be exactly
  `width × height × 4` bytes, compared in 64-bit arithmetic, so a short string is refused
  rather than read past. `paintRgba8()` samples only points inside the image, and its six
  placement numbers must be finite.
- **`pointer()`.** It returns the address of the buffer's bytes for native code to read in
  place. The address is valid while the `FbBuffer` lives; a consumer that keeps it past the
  object's lifetime, through FFI, an `ioctl()` struct or anything else, is outside what ext-fb
  can guard.

A report is in scope when ext-fb itself reads or writes outside memory it should, leaks, or
crashes on any argument: a size computed from sides that can disagree with the allocation, a
row copied past its buffer, a check that runs after the write it guards.
