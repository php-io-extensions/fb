<?php

declare(strict_types=1);

/*
 * The glass kernels against a plain PHP reference of the same arithmetic (core.h): the Kawase level down and up,
 * byte for byte, and the pane: inside the rounded box only, its anti-aliased edge, and a flat pane's colours.
 */

function glassFormat(): FbFormat
{
    return new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA);
}

function glassNoise(int $w, int $h, int $seed): FbBuffer
{
    $b = new FbBuffer(glassFormat(), $w, $h);
    mt_srand($seed);
    for ($y = 0; $y < $h; $y++) {
        for ($x = 0; $x < $w; $x++) {
            $b->set($x, $y, mt_rand(0, 0x7FFFFFFF) * 2 + mt_rand(0, 1));
        }
    }

    return $b;
}

/** @return list<int> channel k of pixel (x, y) of RGBA bytes */
function glassAt(string $bytes, int $w, int $x, int $y, int $k): int
{
    return ord($bytes[($y * $w + $x) * 4 + $k]);
}

function kawaseDownReference(string $src, int $sw, array $r): string
{
    [$rx, $ry, $rw, $rh] = $r;
    [$dw, $dh] = [intdiv($rw + 1, 2), intdiv($rh + 1, 2)];
    $out = '';
    for ($j = 0; $j < $dh; $j++) {
        for ($i = 0; $i < $dw; $i++) {
            for ($k = 0; $k < 4; $k++) {
                [$all, $centre] = [0, 0];
                for ($b = 0; $b < 4; $b++) {
                    for ($a = 0; $a < 4; $a++) {
                        $v = glassAt($src, $sw, max($rx, min($rx + $rw - 1, $rx + 2 * $i - 1 + $a)), max($ry, min($ry + $rh - 1, $ry + 2 * $j - 1 + $b)), $k);
                        $all += $v;
                        $centre += ($a === 1 || $a === 2) && ($b === 1 || $b === 2) ? $v : 0;
                    }
                }
                $out .= chr((4 * $centre + $all + 16) >> 5);
            }
        }
    }

    return $out;
}

function kawaseUpReference(string $src, int $sw, int $sh, int $dw, int $dh): string
{
    $tap = function (int $x4, int $y4, int $k) use ($src, $sw, $sh): int {
        [$ix, $iy, $fx, $fy] = [$x4 >> 2, $y4 >> 2, $x4 & 3, $y4 & 3];
        [$x0, $x1, $y0, $y1] = [max(0, min($sw - 1, $ix)), max(0, min($sw - 1, $ix + 1)), max(0, min($sh - 1, $iy)), max(0, min($sh - 1, $iy + 1))];

        return (4 - $fy) * ((4 - $fx) * glassAt($src, $sw, $x0, $y0, $k) + $fx * glassAt($src, $sw, $x1, $y0, $k))
            + $fy * ((4 - $fx) * glassAt($src, $sw, $x0, $y1, $k) + $fx * glassAt($src, $sw, $x1, $y1, $k));
    };
    $out = '';
    for ($y = 0; $y < $dh; $y++) {
        for ($x = 0; $x < $dw; $x++) {
            [$x4, $y4] = [2 * $x - 1, 2 * $y - 1];
            for ($k = 0; $k < 4; $k++) {
                $sum = $tap($x4 - 4, $y4, $k) + $tap($x4 + 4, $y4, $k) + $tap($x4, $y4 - 4, $k) + $tap($x4, $y4 + 4, $k)
                    + 2 * ($tap($x4 - 2, $y4 - 2, $k) + $tap($x4 + 2, $y4 - 2, $k) + $tap($x4 - 2, $y4 + 2, $k) + $tap($x4 + 2, $y4 + 2, $k));
                $out .= chr(intdiv($sum + 96, 192));
            }
        }
    }

    return $out;
}

it('takes a level down as the reference does, in and off the region\'s edges', function (array $region): void {
    $src = glassNoise(23, 17, 1);
    $down = new FbBuffer(glassFormat(), intdiv($region[2] + 1, 2), intdiv($region[3] + 1, 2));
    $down->kawaseDown($src, ...$region);

    expect(bin2hex($down->bytes()))->toBe(bin2hex(kawaseDownReference($src->bytes(), 23, $region)));
})->with([[[0, 0, 23, 17]], [[3, 2, 11, 9]], [[22, 16, 1, 1]], [[5, 0, 2, 17]]]);

it('takes a level up as the reference does, to twice the size or a pixel less', function (int $dw, int $dh): void {
    $src = glassNoise(7, 5, 2);
    $up = new FbBuffer(glassFormat(), $dw, $dh);
    $up->kawaseUp($src);

    expect(bin2hex($up->bytes()))->toBe(bin2hex(kawaseUpReference($src->bytes(), 7, 5, $dw, $dh)));
})->with([[14, 10], [13, 9], [1, 1]]);

it('keeps a flat colour flat through the pyramid', function (): void {
    $src = new FbBuffer(glassFormat(), 20, 12);
    $src->fill(0x336699CC);
    $down = new FbBuffer(glassFormat(), 10, 6);
    $down->kawaseDown($src, 0, 0, 20, 12);
    $up = new FbBuffer(glassFormat(), 20, 12);
    $up->kawaseUp($down);

    expect(bin2hex($down->bytes()))->toBe(str_repeat('336699cc', 60))
        ->and(bin2hex($up->bytes()))->toBe(str_repeat('336699cc', 240));
});

it('paints a pane inside its rounded box only, with an anti-aliased edge, and a flat pane as its tint', function (): void {
    $target = new FbBuffer(glassFormat(), 24, 16);
    $target->fill(0x000000FF);
    $blur = new FbBuffer(glassFormat(), 12, 8);
    $blur->fill(0xFFFFFFFF);
    // a white backdrop under a half-red tint, no refraction, rim or grain, full opacity
    $bounds = $target->paintGlass([2.0, 2.0, 22.0, 14.0], [6.0, 6.0, 6.0, 6.0], $blur, 2, null, 0, 0, [1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0xFF000080, 0, 255], 0, 0, 24, 16);

    $at = fn (int $x, int $y): int => $target->get($x, $y);
    expect($bounds)->toBe([2, 2, 20, 12])
        ->and(sprintf('%08X', $at(12, 8)))->toBe('FF7F7FFF')     // inside: white mixed half toward red
        ->and(sprintf('%08X', $at(0, 0)))->toBe('000000FF')      // outside the box
        ->and(sprintf('%08X', $at(2, 2)))->toBe('000000FF')      // past the rounded corner
        ->and($at(2, 8) !== 0x000000FF && $at(2, 8) !== 0xFF7F7FFF)->toBeTrue(); // the edge column, half covered
});

it('refuses levels that are not RGBA8888 and sizes that do not match', function (Closure $call, string $message): void {
    expect($call)->toThrow(ValueError::class, $message);
})->with([
    'a level down of the wrong size' => [fn () => (new FbBuffer(glassFormat(), 5, 5))->kawaseDown(glassNoise(8, 8, 3), 0, 0, 8, 8), 'A level down of 8x8 is 4x4, this is 5x5'],
    'a region past the source' => [fn () => (new FbBuffer(glassFormat(), 4, 4))->kawaseDown(glassNoise(8, 8, 3), 1, 0, 8, 8), 'The region is empty or not inside the 8x8 source'],
    'a level up too large' => [fn () => (new FbBuffer(glassFormat(), 9, 4))->kawaseUp(glassNoise(4, 4, 3)), 'A level up of 4x4 is at most 8x8, this is 9x4'],
    'not RGBA8888' => [fn () => (new FbBuffer(new FbFormat(FB_LAYOUT_RGB565), 4, 4))->kawaseDown(glassNoise(8, 8, 3), 0, 0, 8, 8), 'This is not an RGBA8888 buffer'],
    'a short look' => [fn () => (new FbBuffer(glassFormat(), 4, 4))->paintGlass([0, 0, 4, 4], [0, 0, 0, 0], glassNoise(2, 2, 3), 2, null, 0, 0, [1.0], 0, 0, 4, 4), 'The look is a list of 9 numbers'],
]);
