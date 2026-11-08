<?php

declare(strict_types=1);

/*
 * The bulk operations move whole runs of bytes where a layout allows it. Each
 * is held here to the same work done one pixel at a time through get(), set()
 * and the format's mapper, over buffers that start as noise so a stray byte shows.
 */

const BULK_WIDTH = 13;    // odd: the 1, 2, 4 and 12-bit rows end in padding
const BULK_HEIGHT = 19;   // the third page of a paged layout is partial

/** @param array<string, mixed> $arguments FbFormat's constructor arguments by name */
function bulkFormat(array $arguments, int $scan = FB_SCAN_TOP_DOWN): FbFormat
{
    return new FbFormat(...[...$arguments, 'scan' => $scan]);
}

/** Every bit a word of the layout can hold. */
function wordMask(FbFormat $format): int
{
    return match ($format->layout()) {
        FB_LAYOUT_MONO_ROWS, FB_LAYOUT_MONO_PAGES => 1,
        FB_LAYOUT_INDEX2, FB_LAYOUT_PLANAR => 3,
        FB_LAYOUT_INDEX4 => 0xF,
        FB_LAYOUT_INDEX8 => 0xFF,
        FB_LAYOUT_RGB444 => 0xFFF,
        FB_LAYOUT_RGB565 => 0xFFFF,
        FB_LAYOUT_RGB666 => 0xFCFCFC,
        FB_LAYOUT_RGB888 => 0xFFFFFF,
        FB_LAYOUT_RGBA8888 => 0xFFFFFFFF,
    };
}

/** A buffer with a seeded random word in every pixel. */
function noise(FbFormat $format, int $seed, int $width = BULK_WIDTH, int $height = BULK_HEIGHT): FbBuffer
{
    mt_srand($seed);
    $buffer = new FbBuffer($format, $width, $height);
    $mask = wordMask($format);
    for ($y = 0; $y < $height; $y++) {
        for ($x = 0; $x < $width; $x++) {
            $buffer->set($x, $y, ((mt_rand() << 16) ^ mt_rand()) & $mask);
        }
    }

    return $buffer;
}

/** The pixel `from` holds at a point, as `to` stores it. */
function mapped(FbBuffer $from, FbFormat $to, int $x, int $y): int
{
    $rgba = $from->format()->unmapRgba8($from->get($x, $y));

    return $to->mapRgba8($rgba >> 24, ($rgba >> 16) & 0xFF, ($rgba >> 8) & 0xFF, $rgba & 0xFF);
}

dataset('formats', [
    'mono rows' => [['layout' => FB_LAYOUT_MONO_ROWS]],
    'mono rows, lsb first' => [['layout' => FB_LAYOUT_MONO_ROWS, 'bitOrder' => FB_BIT_ORDER_LSB]],
    'mono pages' => [['layout' => FB_LAYOUT_MONO_PAGES, 'bitOrder' => FB_BIT_ORDER_LSB]],
    'index 2' => [['layout' => FB_LAYOUT_INDEX2]],
    'index 4, low nibble first' => [['layout' => FB_LAYOUT_INDEX4, 'bitOrder' => FB_BIT_ORDER_LSB]],
    'index 8' => [['layout' => FB_LAYOUT_INDEX8]],
    'index 8 with a palette' => [['layout' => FB_LAYOUT_INDEX8, 'palette' => [[0x000000, false, 0], [0xFFFFFF, false, 1], [0xFF0000, false, 7]]]],
    'rgb444' => [['layout' => FB_LAYOUT_RGB444]],
    'rgb565' => [['layout' => FB_LAYOUT_RGB565]],
    'bgr565, low byte first' => [['layout' => FB_LAYOUT_RGB565, 'byteOrder' => FB_BYTE_ORDER_LSB, 'channelOrder' => FB_CHANNELS_BGR]],
    'rgb666' => [['layout' => FB_LAYOUT_RGB666]],
    'rgb888' => [['layout' => FB_LAYOUT_RGB888]],
    'bgr888' => [['layout' => FB_LAYOUT_RGB888, 'channelOrder' => FB_CHANNELS_BGR]],
    'rgba8888' => [['layout' => FB_LAYOUT_RGBA8888, 'channelOrder' => FB_CHANNELS_RGBA]],
    'bgra8888' => [['layout' => FB_LAYOUT_RGBA8888, 'channelOrder' => FB_CHANNELS_BGRA]],
    'argb8888' => [['layout' => FB_LAYOUT_RGBA8888, 'channelOrder' => FB_CHANNELS_ARGB]],
    'abgr8888' => [['layout' => FB_LAYOUT_RGBA8888, 'channelOrder' => FB_CHANNELS_ABGR]],
    'planar, black inverted' => [['layout' => FB_LAYOUT_PLANAR, 'palette' => [[0x000000, true, 0], [0xFF0000, false, 1]]]],
]);

dataset('rects', [
    'the whole buffer' => [[0, 0, BULK_WIDTH, BULK_HEIGHT]],
    'a band of whole rows' => [[0, 3, BULK_WIDTH, 5]],
    'two whole pages' => [[0, 0, BULK_WIDTH, 16]],
    'part of one page' => [[3, 8, 7, 8]],
    'an interior rect' => [[2, 1, 9, 13]],
    'the last row' => [[0, BULK_HEIGHT - 1, BULK_WIDTH, 1]],
    'one column' => [[5, 0, 1, BULK_HEIGHT]],
    'the last pixel' => [[BULK_WIDTH - 1, BULK_HEIGHT - 1, 1, 1]],
]);

it('fills a rect to what set() writes pixel by pixel', function (array $arguments, array $rect): void {
    [$left, $top, $width, $height] = $rect;
    $format = bulkFormat($arguments);
    $filled = noise($format, 1);
    $expected = noise($format, 1);

    foreach ([0xA5C3E1B7, 0, 0xFFFFFFFF] as $value) {
        $word = $value & wordMask($format);
        $filled->rect($left, $top, $width, $height, $word);
        for ($y = $top; $y < $top + $height; $y++) {
            for ($x = $left; $x < $left + $width; $x++) {
                $expected->set($x, $y, $word);
            }
        }

        expect(bin2hex($filled->bytes()))->toBe(bin2hex($expected->bytes()));
    }
})->with('formats', 'rects');

it('fills the whole buffer to what set() writes pixel by pixel', function (array $arguments): void {
    $format = bulkFormat($arguments);
    $word = 0x5A3C1E87 & wordMask($format);
    $filled = noise($format, 1);
    $filled->fill($word);
    $expected = new FbBuffer($format, BULK_WIDTH, BULK_HEIGHT);
    for ($y = 0; $y < BULK_HEIGHT; $y++) {
        for ($x = 0; $x < BULK_WIDTH; $x++) {
            $expected->set($x, $y, $word);
        }
    }

    expect(bin2hex($filled->bytes()))->toBe(bin2hex($expected->bytes()));
})->with('formats');

it('copies a rect to what get() and set() move pixel by pixel', function (array $arguments, array $rect): void {
    [$left, $top, $width, $height] = $rect;
    $format = bulkFormat($arguments);
    $source = noise($format, 3);
    $target = noise($format, 2);
    $expected = noise($format, 2);

    $target->copy($source, $left, $top, $width, $height);
    for ($y = $top; $y < $top + $height; $y++) {
        for ($x = $left; $x < $left + $width; $x++) {
            $expected->set($x, $y, $source->get($x, $y));
        }
    }

    expect(bin2hex($target->bytes()))->toBe(bin2hex($expected->bytes()));
})->with('formats', 'rects');

it('answers a region as stored, in either scan order, to what get() reads pixel by pixel', function (array $arguments, array $rect): void {
    [$left, $top, $width, $height] = $rect;

    foreach ([FB_SCAN_TOP_DOWN, FB_SCAN_BOTTOM_UP] as $scan) {
        $format = bulkFormat($arguments, $scan);
        $buffer = noise($format, 4);
        $expected = new FbBuffer($format, $width, $height);
        for ($y = 0; $y < $height; $y++) {
            for ($x = 0; $x < $width; $x++) {
                $expected->set($x, $scan === FB_SCAN_BOTTOM_UP ? $height - 1 - $y : $y, $buffer->get($left + $x, $top + $y));
            }
        }

        expect(bin2hex($buffer->region($left, $top, $width, $height)))->toBe(bin2hex($expected->bytes()));
    }
})->with('formats', 'rects');

it('maps a region into another format to what the mapper gives pixel by pixel', function (array $from, array $to): void {
    $buffer = noise(bulkFormat($from), 5);

    foreach ([[0, 0, BULK_WIDTH, BULK_HEIGHT], [2, 1, 9, 13]] as [$left, $top, $width, $height]) {
        foreach ([FB_SCAN_TOP_DOWN, FB_SCAN_BOTTOM_UP] as $scan) {
            $format = bulkFormat($to, $scan);
            $expected = new FbBuffer($format, $width, $height);
            for ($y = 0; $y < $height; $y++) {
                for ($x = 0; $x < $width; $x++) {
                    $expected->set($x, $scan === FB_SCAN_BOTTOM_UP ? $height - 1 - $y : $y, mapped($buffer, $format, $left + $x, $top + $y));
                }
            }

            expect(bin2hex($buffer->region($left, $top, $width, $height, $format)))->toBe(bin2hex($expected->bytes()));
        }
    }
})->with('formats', 'formats');

it('keeps mapping a palette through its inks when the other format stores it the same way', function (): void {
    $palette = [[0x000000, false, 0], [0xFFFFFF, false, 1], [0xFF0000, false, 7]];
    $buffer = new FbBuffer(new FbFormat(FB_LAYOUT_INDEX8, palette: $palette), 2, 1);
    $buffer->setPixels([[0, 0, 7], [1, 0, 200]]);   // 200 is no ink: it reads as paper

    expect(bin2hex($buffer->region(0, 0, 2, 1)))->toBe('07c8')
        ->and(bin2hex($buffer->region(0, 0, 2, 1, new FbFormat(FB_LAYOUT_INDEX8, palette: $palette))))->toBe('0701');
});

it('answers RGBA8 to what the mapper gives pixel by pixel', function (array $arguments): void {
    $buffer = noise(bulkFormat($arguments), 6);
    $expected = '';
    for ($y = 0; $y < BULK_HEIGHT; $y++) {
        for ($x = 0; $x < BULK_WIDTH; $x++) {
            $expected .= pack('N', $buffer->format()->unmapRgba8($buffer->get($x, $y)));
        }
    }

    expect(bin2hex($buffer->rgba8()))->toBe(bin2hex($expected));
})->with('formats');

it('maps RGBA8 pixels in to what the mapper gives pixel by pixel', function (array $arguments, int $at_x, int $at_y, array $clip): void {
    $format = bulkFormat($arguments);
    $blitted = noise($format, 7);
    $expected = noise($format, 7);
    [$source_width, $source_height] = [9, 7];
    mt_srand(8);
    $source = '';
    for ($i = 0; $i < $source_width * $source_height * 4; $i++) {
        $source .= chr(mt_rand(0, 255));
    }
    [$clip_x, $clip_y, $clip_width, $clip_height] = $clip;
    $left = max($at_x, 0, $clip_x);
    $top = max($at_y, 0, $clip_y);
    $right = min($at_x + $source_width, BULK_WIDTH, $clip_x + ($clip_width ?? BULK_WIDTH));
    $bottom = min($at_y + $source_height, BULK_HEIGHT, $clip_y + ($clip_height ?? BULK_HEIGHT));

    $written = $blitted->blitRgba8($source, $source_width, $source_height, $at_x, $at_y, $clip_x, $clip_y, $clip_width, $clip_height);
    for ($y = $top; $y < $bottom; $y++) {
        for ($x = $left; $x < $right; $x++) {
            [$red, $green, $blue, $alpha] = array_values(unpack('C4', $source, (($y - $at_y) * $source_width + ($x - $at_x)) * 4));
            $expected->set($x, $y, $format->mapRgba8($red, $green, $blue, $alpha));
        }
    }

    expect($written)->toBe([$left, $top, $right - $left, $bottom - $top])
        ->and(bin2hex($blitted->bytes()))->toBe(bin2hex($expected->bytes()));
})->with('formats')->with([
    'inside' => [2, 3, [0, 0, null, null]],
    'over the top left corner' => [-4, -2, [0, 0, null, null]],
    'over the bottom right corner' => [8, 15, [0, 0, null, null]],
    'across whole rows' => [-1, 4, [0, 0, null, null]],
    'through a clip rect' => [1, 2, [3, 4, 5, 2]],
]);

/** RGB layouts and palette-less greys blend; mono, palette and planar layouts threshold at 128. */
function blends(array $arguments): bool
{
    return ! in_array($arguments['layout'], [FB_LAYOUT_MONO_ROWS, FB_LAYOUT_MONO_PAGES, FB_LAYOUT_PLANAR], true) && ! isset($arguments['palette']);
}

/**
 * The spans painted pixel by pixel with get(), set() and the mapper: the rule paintSpans() keeps.
 *
 * @param list<array{int, int, int, int}> $list [y, x, length, coverage]
 */
function paintByPixel(FbBuffer $expected, FbFormat $format, array $arguments, array $list, int $colour): void
{
    [$red, $green, $blue, $alpha] = [$colour >> 24, ($colour >> 16) & 0xFF, ($colour >> 8) & 0xFF, $colour & 0xFF];
    foreach ($list as [$y, $x0, $length, $coverage]) {
        $a = intdiv($alpha * $coverage + 127, 255);
        for ($x = $x0; $x < $x0 + $length; $x++) {
            if ($a === 255 || (! blends($arguments) && $a >= 128)) {
                $expected->set($x, $y, $format->mapRgba8($red, $green, $blue, $alpha));
            } elseif ($a > 0 && blends($arguments)) {
                $d = $format->unmapRgba8($expected->get($x, $y));
                $expected->set($x, $y, $format->mapRgba8(
                    intdiv($red * $a + ($d >> 24) * (255 - $a) + 127, 255),
                    intdiv($green * $a + (($d >> 16) & 0xFF) * (255 - $a) + 127, 255),
                    intdiv($blue * $a + (($d >> 8) & 0xFF) * (255 - $a) + 127, 255),
                    intdiv(255 * $a + ($d & 0xFF) * (255 - $a) + 127, 255),
                ));
            }
        }
    }
}

it('paints spans to what the mapper and the blend rule give pixel by pixel', function (array $arguments): void {
    $format = bulkFormat($arguments);
    $painted = noise($format, 10);
    $expected = noise($format, 10);
    mt_srand(11);

    for ($round = 0; $round < 4; $round++) {
        $colour = (mt_rand() << 16 ^ mt_rand()) & 0xFFFFFFFF;
        $spans = '';
        $list = [];
        for ($y = 0; $y < BULK_HEIGHT; $y++) {
            for ($x = 0; $x < BULK_WIDTH;) {
                $length = mt_rand(1, BULK_WIDTH - $x);
                $coverage = [0, 1, 127, 128, 254, 255, mt_rand(0, 255)][mt_rand(0, 6)];
                if (mt_rand(0, 2) > 0) {
                    $spans .= pack('vvvC', $y, $x, $length, $coverage);
                    $list[] = [$y, $x, $length, $coverage];
                }
                $x += $length;
            }
        }

        $painted->paintSpans($spans, $colour);
        paintByPixel($expected, $format, $arguments, $list, $colour);

        expect(bin2hex($painted->bytes()))->toBe(bin2hex($expected->bytes()), "round {$round}");
    }
})->with('formats');

it('paints opaque, short and long runs to what the mapper and the blend rule give pixel by pixel', function (array $arguments): void {
    $format = bulkFormat($arguments);
    [$painted, $expected] = [noise($format, 12, 80, 9), noise($format, 12, 80, 9)];

    foreach ([0x3366CCFF, 0xC0A060B0, 0x10F0807F, 0xFFFFFF01] as $round => $colour) {
        [$spans, $list] = ['', []];
        foreach ([[1, 0, 255], [2, 3, 128], [3, 9, 200], [31, 20, 255], [32, 1, 90], [33, 40, 255], [79, 0, 37], [80, 0, 255]] as $row => [$length, $x, $coverage]) {
            $length = min($length, 80 - $x);
            $spans .= pack('vvvC', $row, $x, $length, $coverage);
            $list[] = [$row, $x, $length, $coverage];
        }
        $painted->paintSpans($spans, $colour);
        paintByPixel($expected, $format, $arguments, $list, $colour);

        expect(bin2hex($painted->bytes()))->toBe(bin2hex($expected->bytes()), "round {$round}");
    }
})->with('formats');

/**
 * The spans that paint what masks paint: per mask in order, per row inside the clip, a span per run of one coverage
 * above 0, cut to the clip.
 *
 * @param list<array{string, int, int, int}> $masks
 * @param array{int, int, int, int} $clip
 */
function maskSpans(array $masks, array $clip): string
{
    [$cx, $cy, $cw, $ch] = $clip;
    $spans = '';
    foreach ($masks as [$coverage, $width, $left, $top]) {
        $height = intdiv(strlen($coverage), $width);
        for ($row = 0; $row < $height; $row++) {
            $y = $top + $row;
            if ($y < $cy || $y >= $cy + $ch) {
                continue;
            }
            for ($col = 0; $col < $width;) {
                $c = ord($coverage[$row * $width + $col]);
                $end = $col + 1;
                while ($end < $width && ord($coverage[$row * $width + $end]) === $c) {
                    $end++;
                }
                $x0 = max($left + $col, $cx);
                $x1 = min($left + $end, $cx + $cw);
                if ($c > 0 && $x1 > $x0) {
                    $spans .= pack('vvvC', $y, $x0, $x1 - $x0, $c);
                }
                $col = $end;
            }
        }
    }

    return $spans;
}

/** @return list<array{string, int, int, int}> Masks of coverage runs, overlapping, some past the buffer's edges. */
function randomMasks(int $count): array
{
    $masks = [];
    for ($i = 0; $i < $count; $i++) {
        $width = mt_rand(1, 9);
        $height = mt_rand(1, 7);
        $coverage = '';
        for ($k = 0; $k < $width * $height;) {
            $run = mt_rand(1, 4);
            $coverage .= str_repeat(chr([0, 0, 1, 127, 128, 254, 255, 255, mt_rand(0, 255)][mt_rand(0, 8)]), $run);
            $k += $run;
        }
        $masks[] = [substr($coverage, 0, $width * $height), $width, mt_rand(-4, BULK_WIDTH), mt_rand(-4, BULK_HEIGHT)];
    }

    return $masks;
}

it('paints masks as paintSpans paints the same runs: overlap in order, cut to the clip, the same bounds', function (array $arguments): void {
    $format = bulkFormat($arguments);
    $masked = noise($format, 20);
    $spanned = noise($format, 20);
    mt_srand(21);

    foreach ([[0, 0, BULK_WIDTH, BULK_HEIGHT], [2, 3, 7, 9], [BULK_WIDTH - 1, 0, 1, BULK_HEIGHT]] as $round => $clip) {
        $colour = (mt_rand() << 16 ^ mt_rand()) & 0xFFFFFFFF;
        $masks = randomMasks(12);
        $box = $masked->paintMasks($masks, $colour, ...$clip);
        $spans = maskSpans($masks, $clip);
        $expected = $spans === '' ? null : $spanned->paintSpans($spans, $colour);

        expect($box)->toBe($expected, "round {$round}")
            ->and(bin2hex($masked->bytes()))->toBe(bin2hex($spanned->bytes()), "round {$round}");
    }
})->with('formats');

it('paints nothing and answers null for masks off the clip or of no coverage', function (): void {
    $buffer = noise(bulkFormat(['layout' => FB_LAYOUT_RGBA8888, 'channelOrder' => FB_CHANNELS_RGBA]), 30);
    $before = $buffer->bytes();

    expect($buffer->paintMasks([], 0xFF0000FF, 0, 0, BULK_WIDTH, BULK_HEIGHT))->toBeNull()
        ->and($buffer->paintMasks([["\xFF\xFF", 2, 0, 0]], 0xFF0000FF, 4, 4, 3, 3))->toBeNull()
        ->and($buffer->paintMasks([[str_repeat("\0", 6), 3, 0, 0]], 0xFF0000FF, 0, 0, BULK_WIDTH, BULK_HEIGHT))->toBeNull()
        ->and($buffer->bytes())->toBe($before);
});

it('refuses a bad clip, colour or mask before painting any', function (array $masks, int $colour, array $clip, string $message): void {
    $buffer = noise(bulkFormat(['layout' => FB_LAYOUT_RGBA8888, 'channelOrder' => FB_CHANNELS_RGBA]), 31);
    $before = $buffer->bytes();

    expect(fn () => $buffer->paintMasks($masks, $colour, ...$clip))->toThrow(ValueError::class, $message)
        ->and($buffer->bytes())->toBe($before);
})->with([
    'clip off the buffer' => [[["\xFF", 1, 0, 0]], 0xFF, [0, 0, BULK_WIDTH + 1, 1], 'The clip is empty or not inside'],
    'empty clip' => [[["\xFF", 1, 0, 0]], 0xFF, [0, 0, 0, 1], 'The clip is empty or not inside'],
    'colour past 32 bits' => [[["\xFF", 1, 0, 0]], 0x1_0000_0000, [0, 0, 1, 1], 'takes a colour 0xRRGGBBAA'],
    'a mask that is not a list of four' => [[["\xFF", 1, 0, 0], ["\xFF", 1, 0]], 0xFF, [0, 0, 1, 1], 'Mask 1 is not'],
    'width that splits no rows' => [[["\xFF\xFF\xFF", 2, 0, 0]], 0xFF, [0, 0, 1, 1], 'Mask 0 is not whole rows'],
    'no coverage bytes' => [[['', 1, 0, 0]], 0xFF, [0, 0, 1, 1], 'Mask 0 is not whole rows'],
    'placed too far off' => [[["\xFF", 1, 1 << 31, 0]], 0xFF, [0, 0, 1, 1], 'Mask 0 is placed past'],
]);
