---
type: Runbook
title: Build, install, test
description: Scratch build with gen_stub, the two installers, Pest, the Pi loop.
resource: install-macos.sh
tags: [build, linux, macos, pest]
status: draft
generated: { by: claude-opus/5.5, at: 2026-10-03T21:08:45Z }
sources:
  - id: mac
    resource: install-macos.sh
    title: install-macos.sh
  - id: debian
    resource: install-debian-trixie.sh
    title: install-debian-trixie.sh
---

# Overview

No system library needed: php-dev and a C compiler.

Dev loop: copy `config.m4 php_fb.h src stubs` to a scratch dir, `phpize`, `php build/gen_stub.php stubs`, `./configure --enable-fb --with-php-config=…`, `make`. Copy `stubs/*_arginfo.h` back after a stub edit. Run Pest with `-d extension=<scratch>/modules/fb.so`. Build with `-Wall -Wextra`; no warning from `src/` is acceptable.

macOS (`install-macos.sh`): builds in a temp copy for php@8.4 and php@8.4-zts, ad-hoc signs, writes `30-fb.ini`.[^mac]

Linux (`install-debian-trixie.sh`): builds in place, installs `fb.so`, writes `30-fb.ini`, removes build output.[^debian]

Pi from the Mac: tree on the Mac is authoritative, Pi copy disposable. `COPYFILE_DISABLE=1 tar --no-mac-metadata --exclude .git -czf - -C <ext> . | fnk 'tar -xzf - -C ~/fb'`, install, Pest, `rm -rf ~/fb`. Without `--no-mac-metadata` the copy carries `._*` files that gen_stub trips on.

A PHP can carry the 0.8 Zephir ext, also named `fb`. Loading both warns `Module "fb" is already loaded` and the old one wins: install over it, or test with a `PHP_INI_SCAN_DIR` that omits `30-fb.ini`.

[^mac]: Same script as the gtk ext's, GTK checks removed.
[^debian]: Same script as the gtk ext's, GTK checks removed.
