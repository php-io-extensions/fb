#!/usr/bin/env php
<?php
declare(strict_types=1);

if (! extension_loaded('fb')) { fwrite(STDERR, "fb not loaded\n"); exit(1); }

$dir = getenv('FB_FIXTURES') ?: dirname(__DIR__, 2).'/../../venusian/surface/tests/Framebuffers/fixtures';
$files = glob($dir.'/*.json'); sort($files);

$PF = ['mono_vertical_page' => 0, 'mono_horizontal' => 1, 'row_major' => 2, 'planar' => 3];
$EINK = [0 => 0xFFFFFF, 1 => 0x000000, 2 => 0xFF0000, 3 => 0xFFFF00, 4 => 0x0000FF, 5 => 0x00FF00, 6 => 0xFF8000];
$KIND = ['full' => 0, 'dirty' => 1, 'epaper' => 2, 'paged' => 3, 'ring' => 4];

function fmtArgs(array $f, array $PF, array $EINK): array
{
    $palette = [];
    foreach ($f['palette'] ?? [] as $i => $c) {
        $palette[] = [$EINK[$c['color']], (int) ($c['inverted'] ?? false), $c['code'] ?? $i];
    }
    return [
        $PF[$f['pixel_format']], $f['bit_depth'],
        isset($f['bit_order']) ? ($f['bit_order'] === 'msb_first' ? 0 : 1) : -1,
        isset($f['endianness']) ? ($f['endianness'] === 'lsb' ? 0 : 1) : -1,
        isset($f['page_axis']) ? ($f['page_axis'] === 'horizontal' ? 0 : 1) : -1,
        ($f['scan'] ?? 'top_to_bottom') === 'bottom_to_top' ? 1 : 0,
        $palette,
    ];
}

function fail(string $m): never { fwrite(STDERR, $m."\n"); exit(1); }

$count = 0;
foreach ($files as $file) {
    $fx = json_decode(file_get_contents($file), true, 512, JSON_THROW_ON_ERROR);
    [$pf, $bd, $bo, $en, $pa, $sc, $pal] = fmtArgs($fx['format'], $PF, $EINK);
    $extra = $fx['page_rows'] ?? $fx['frames'] ?? 0;
    $h = Fb\Buffer::create($KIND[$fx['kind']], $pf, $bd, $bo, $en, $pa, $sc, $pal, $fx['width'], $fx['height'], $extra);
    if ($h === 0) fail("{$fx['name']}: create refused");
    foreach ($fx['steps'] as $i => $step) {
        foreach ($step['ops'] ?? [] as $op) {
            match ($op[0]) {
                'set' => Fb\Buffer::set($h, $op[1], $op[2], $op[3]),
                'segment' => Fb\Buffer::setSegment($h, $op[1], $op[2], $op[3], $op[4], $op[5]),
                'fill' => Fb\Buffer::fill($h, $op[1]),
                'clear' => Fb\Buffer::fill($h, 0),
                'pixels' => Fb\Buffer::setPixels($h, $op[1]),
                'region_set' => Fb\Buffer::setPixels($h, array_map(fn ($c) => [$c[0], $c[1], $op[2]], $op[1])),
                'rgba8' => Fb\Buffer::blitRgba8($h, hex2bin($op[1]), $fx['width'], $fx['height'], 0, 0),
                'epoch' => Fb\Buffer::beginEpoch($h),
                'page' => Fb\Buffer::setPage($h, $op[1]),
                'present' => Fb\Buffer::present($h),
            };
        }
        $e = $step['expect'] ?? [];
        $at = "{$fx['name']} step {$i}";
        if (isset($e['bytes_hex'])) {
            $got = bin2hex(Fb\Buffer::region($h, 0, 0, $fx['width'], $fx['height'], $sc));
            if ($fx['kind'] === 'paged') { $got = bin2hex(Fb\Buffer::bytes($h)); }
            if ($got !== $e['bytes_hex']) fail("{$at}: bytes {$got} != {$e['bytes_hex']}");
        }
        foreach ($e['get'] ?? [] as [$x, $y, $v]) {
            if (Fb\Buffer::get($h, $x, $y) !== $v) fail("{$at}: get({$x},{$y})");
        }
        if (isset($e['region'])) {
            [[$x, $y, $w, $hh], $hex] = $e['region'];
            $got = bin2hex(Fb\Buffer::region($h, $x, $y, $w, $hh, $sc));
            if ($got !== $hex) fail("{$at}: region {$got} != {$hex}");
        }
        if (isset($e['rgba8_hex']) && bin2hex(Fb\Buffer::toRgba8($h)) !== $e['rgba8_hex']) fail("{$at}: rgba8");
        if (isset($e['damage']) && Fb\Buffer::damage($h) !== $e['damage']) fail("{$at}: damage ".json_encode(Fb\Buffer::damage($h)));
        if (isset($e['pages']) && Fb\Buffer::pages($h) !== $e['pages']) fail("{$at}: pages");
        if (isset($e['page']) && Fb\Buffer::page($h) !== $e['page']) fail("{$at}: page");
        if (isset($e['granularity']) && Fb\Buffer::granularity($h) !== $e['granularity']) fail("{$at}: granularity");
        foreach ($e['flush'] ?? [] as $f) {
            [$tpf, $tbd, $tbo, $ten, $tpa, $tsc, $tpal] = fmtArgs($f['format'], $PF, $EINK);
            $got = bin2hex(Fb\Buffer::transcode($h, $tpf, $tbd, $tbo, $ten, $tpa, $tsc, $tpal, 0, 0, $fx['width'], $fx['height']));
            if ($got !== $f['bytes_hex']) fail("{$at}: transcode {$got} != {$f['bytes_hex']}");
        }
    }
    Fb\Buffer::free($h);
    $count++;
}
echo "FB_FIXTURES_OK {$count}\n";
