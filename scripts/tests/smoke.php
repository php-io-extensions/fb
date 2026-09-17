#!/usr/bin/env php
<?php
declare(strict_types=1);

if (! extension_loaded('fb')) { fwrite(STDERR, "fb not loaded\n"); exit(1); }

$h = Fb\Buffer::create(0, 1, 1, 0, -1, -1, 0, [], 8, 2, 0);   // full, mono horizontal MSB, 8x2
if ($h === 0) { fwrite(STDERR, "create failed\n"); exit(1); }
Fb\Buffer::set($h, 0, 0, 1);
Fb\Buffer::set($h, 7, 0, 1);
Fb\Buffer::setSegment($h, 2, 1, 4, 1, 1);
$bytes = bin2hex(Fb\Buffer::bytes($h));
Fb\Buffer::free($h);
if ($bytes !== '813c') { fwrite(STDERR, "expected 813c got {$bytes}\n"); exit(1); }
echo "FB_SMOKE_OK\n";
