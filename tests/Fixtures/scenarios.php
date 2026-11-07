<?php

declare(strict_types=1);

/*
 * The blend scenarios: each a name and a closure answering the bytes it paints. capture.php
 * records their SHA-1 from a build before the table-driven blend (blend.json); the tests
 * hold every later build to those bytes.
 */

/** @return array<string, array{FbFormat}> name => format, every layout and order BulkTest covers */
function blendFormats(): array
{
    $make = fn (int $layout, int $bitOrder = FB_BIT_ORDER_MSB, int $byteOrder = FB_BYTE_ORDER_MSB, int $channels = FB_CHANNELS_RGB, array $palette = []) => new FbFormat($layout, $bitOrder, $byteOrder, $channels, FB_SCAN_TOP_DOWN, $palette);

    return [
        'mono rows' => $make(FB_LAYOUT_MONO_ROWS),
        'mono pages' => $make(FB_LAYOUT_MONO_PAGES, FB_BIT_ORDER_LSB),
        'index 2' => $make(FB_LAYOUT_INDEX2),
        'index 4' => $make(FB_LAYOUT_INDEX4, FB_BIT_ORDER_LSB),
        'index 8' => $make(FB_LAYOUT_INDEX8),
        'index 8 palette' => $make(FB_LAYOUT_INDEX8, palette: [[0x000000, false, 0], [0xFFFFFF, false, 1], [0xFF0000, false, 7]]),
        'rgb444' => $make(FB_LAYOUT_RGB444),
        'rgb565' => $make(FB_LAYOUT_RGB565),
        'bgr565 lsb' => $make(FB_LAYOUT_RGB565, byteOrder: FB_BYTE_ORDER_LSB, channels: FB_CHANNELS_BGR),
        'rgb666' => $make(FB_LAYOUT_RGB666),
        'rgb888' => $make(FB_LAYOUT_RGB888),
        'bgr888' => $make(FB_LAYOUT_RGB888, channels: FB_CHANNELS_BGR),
        'rgba8888' => $make(FB_LAYOUT_RGBA8888, channels: FB_CHANNELS_RGBA),
        'bgra8888' => $make(FB_LAYOUT_RGBA8888, channels: FB_CHANNELS_BGRA),
        'argb8888' => $make(FB_LAYOUT_RGBA8888, channels: FB_CHANNELS_ARGB),
        'abgr8888' => $make(FB_LAYOUT_RGBA8888, channels: FB_CHANNELS_ABGR),
        'planar' => $make(FB_LAYOUT_PLANAR, palette: [[0x000000, true, 0], [0xFF0000, false, 1]]),
    ];
}

function blendSpan(int $y, int $x, int $length, int $coverage): string
{
    return pack('vvvC', $y, $x, $length, $coverage);
}

/** A buffer of seeded random colours, each set through the format's own mapping. */
function blendNoise(FbFormat $format, int $width, int $height, int $seed): FbBuffer
{
    $random = new Random\Randomizer(new Random\Engine\Mt19937($seed));
    $buffer = new FbBuffer($format, $width, $height);
    for ($y = 0; $y < $height; $y++) {
        for ($x = 0; $x < $width; $x++) {
            $buffer->set($x, $y, $format->mapRgba8($random->getInt(0, 255), $random->getInt(0, 255), $random->getInt(0, 255), $random->getInt(0, 255)));
        }
    }

    return $buffer;
}

/** @return array<string, Closure(): string> Task 1: one span over a 256-pixel ramp, every colour level c and every alpha a */
function blendTableScenarios(): array
{
    $out = [];
    foreach ([0, 1, 127, 128, 254, 255] as $c) {
        for ($a = 0; $a < 256; $a++) {
            $out["table c{$c} a{$a}"] = function () use ($c, $a): string {
                $buffer = new FbBuffer(new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA), 256, 1);
                for ($x = 0; $x < 256; $x++) {
                    $buffer->set($x, 0, $x * 0x01010101);
                }
                // colour alpha 255, coverage a: the effective alpha is (255·a + 127) / 255 = a
                $buffer->paintSpans(blendSpan(0, 0, 256, $a), ($c << 24) | ($c << 16) | ($c << 8) | 0xFF);

                return $buffer->bytes();
            };
        }
    }

    return $out;
}

/** @return array<string, Closure(): string> Task 2: every format, random spans, edge spans, 256 coverages in one call, whole rows */
function blendSpanScenarios(): array
{
    $out = [];
    foreach (blendFormats() as $name => $format) {
        $out["spans {$name} random"] = function () use ($format): string {
            $buffer = blendNoise($format, 40, 8, 11);
            $random = new Random\Randomizer(new Random\Engine\Mt19937(12));
            for ($call = 0; $call < 20; $call++) {
                $spans = '';
                for ($i = 0; $i < 10; $i++) {
                    $x = $random->getInt(0, 39);
                    $spans .= blendSpan($random->getInt(0, 7), $x, $random->getInt(1, 40 - $x), $random->getInt(0, 255));
                }
                $buffer->paintSpans($spans, $random->getInt(0, 0xFFFFFFFF) & 0xFFFFFFFF);
            }

            return $buffer->bytes();
        };
        $out["spans {$name} edges"] = function () use ($format): string {
            $buffer = blendNoise($format, 40, 8, 21);
            foreach ([[0, 0, 40], [1, 0, 1], [2, 39, 1], [3, 20, 20], [4, 0, 33], [5, 7, 33], [6, 38, 2], [7, 0, 2]] as $k => [$y, $x, $length]) {
                foreach ([0, 1, 107, 128, 254, 255] as $alpha) {
                    $buffer->paintSpans(blendSpan($y, $x, $length, 255).blendSpan(($y + 1) % 8, $x, $length, 200), 0x3366CC00 | $alpha);
                }
            }

            return $buffer->bytes();
        };
        $out["spans {$name} 256 coverages"] = function () use ($format): string {
            $buffer = blendNoise($format, 40, 8, 31);
            $spans = '';
            for ($c = 0; $c < 256; $c++) {
                $spans .= blendSpan(intdiv($c, 40), $c % 40, 1, $c);
            }
            $buffer->paintSpans($spans, 0xC8641EC8);

            return $buffer->bytes();
        };
        $out["spans {$name} rows"] = function () use ($format): string {
            $buffer = blendNoise($format, 40, 8, 41);
            foreach ([0x0000006B, 0xFFFFFF01, 0x80402080, 0x12345678] as $rgba) {
                $spans = '';
                for ($y = 0; $y < 8; $y++) {
                    $spans .= blendSpan($y, 0, 40, [255, 255, 128, 3, 255, 77, 255, 254][$y]);
                }
                $buffer->paintSpans($spans, $rgba);
            }

            return $buffer->bytes();
        };
    }

    return $out;
}

/** @return array<string, Closure(): string> Task 3: a seeded source placed by identity, scale, turn and mirror, both filters and two opacities */
function blendImageScenarios(): array
{
    $random = new Random\Randomizer(new Random\Engine\Mt19937(51));
    $source = $random->getBytes(37 * 23 * 4);
    $t = 0.3;
    $inverses = [
        'identity' => [1.0, 0.0, 0.0, 1.0, -5.0, -3.0],
        'scale 2.5' => [0.4, 0.0, 0.0, 0.4, 0.0, 0.0],
        'turn 0.3' => [cos($t), -sin($t), sin($t), cos($t), -4.0, 6.0],
        'mirror' => [-1.0, 0.0, 0.0, 1.0, 42.0, 0.0],
    ];
    $targets = [
        'rgba8888' => new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA),
        'rgb565' => new FbFormat(FB_LAYOUT_RGB565),
    ];
    $out = [];
    foreach ($inverses as $how => $inverse) {
        foreach ($targets as $name => $format) {
            foreach ([false, true] as $smooth) {
                foreach ([255, 128] as $opacity) {
                    $out[sprintf('image %s %s %s %d', $how, $name, $smooth ? 'smooth' : 'nearest', $opacity)] = function () use ($format, $source, $inverse, $smooth, $opacity): string {
                        $buffer = blendNoise($format, 60, 40, 61);
                        $buffer->paintRgba8($source, 37, 23, $inverse, 0, 0, 60, 40, $opacity, $smooth);
                        $buffer->paintRgba8($source, 37, 23, $inverse, 3, 2, 50, 30, $opacity, $smooth, 7);

                        return $buffer->bytes();
                    };
                }
            }
        }
    }

    return $out;
}

/** @return array<string, string> name => expected SHA-1 */
function blendExpected(): array
{
    return json_decode((string) file_get_contents(__DIR__.'/blend.json'), true, flags: JSON_THROW_ON_ERROR);
}
