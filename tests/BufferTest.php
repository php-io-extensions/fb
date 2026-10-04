<?php

declare(strict_types=1);

it('starts blank and knows its shape', function (): void {
    $format = new FbFormat(FB_LAYOUT_RGB565);
    $buffer = new FbBuffer($format, 3, 2);

    expect($buffer->width())->toBe(3)
        ->and($buffer->height())->toBe(2)
        ->and($buffer->size())->toBe(12)
        ->and($buffer->format())->toBe($format)
        ->and($buffer->bytes())->toBe(str_repeat("\0", 12))
        ->and($buffer->granularity())->toBe([1, 1])
        ->and(buffer(FB_LAYOUT_MONO_PAGES, 5, 16)->granularity())->toBe([5, 8])
        ->and($buffer->pointer())->not->toBe(0)
        ->and($buffer->pointer())->not->toBe((new FbBuffer($format, 3, 2))->pointer())
        ->and(bin2hex((new FbBuffer(bwr(), 8, 1))->bytes()))->toBe('ff00');   // an inverted plane's paper is 1
});

it('packs each layout the way its panel takes it', function (FbFormat $format, int $width, array $writes, string $hex): void {
    $buffer = new FbBuffer($format, $width, 2);
    foreach ($writes as [$x, $y, $word]) {
        $buffer->set($x, $y, $word);
    }

    expect(bin2hex($buffer->bytes()))->toBe($hex);
    foreach ($writes as [$x, $y, $word]) {
        expect($buffer->get($x, $y))->toBe($word);
    }
})->with([
    'mono rows, msb first' => [fn () => new FbFormat(FB_LAYOUT_MONO_ROWS), 8, [[0, 0, 1], [7, 0, 1], [2, 1, 1]], '8120'],
    'mono rows, lsb first, padded' => [fn () => new FbFormat(FB_LAYOUT_MONO_ROWS, FB_BIT_ORDER_LSB), 9, [[0, 0, 1], [8, 1, 1]], '01000001'],
    'mono pages, bit 0 on top' => [fn () => new FbFormat(FB_LAYOUT_MONO_PAGES, FB_BIT_ORDER_LSB), 2, [[0, 0, 1], [1, 1, 1]], '0102'],
    'index 2, high crumb first' => [fn () => new FbFormat(FB_LAYOUT_INDEX2), 4, [[0, 0, 3], [3, 0, 1], [1, 1, 2]], 'c120'],
    'index 4, low nibble first' => [fn () => new FbFormat(FB_LAYOUT_INDEX4, FB_BIT_ORDER_LSB), 2, [[0, 0, 0xA], [1, 0, 0x5]], '5a00'],
    'rgb444, two pixels in three bytes' => [fn () => new FbFormat(FB_LAYOUT_RGB444), 3, [[0, 0, 0x123], [1, 0, 0x456], [2, 0, 0x789]], '123456789000000000000000'],
    'bgr444' => [fn () => new FbFormat(FB_LAYOUT_RGB444, channelOrder: FB_CHANNELS_BGR), 2, [[0, 0, 0x123], [1, 0, 0x456]], '321654000000'],
    'rgb565, high byte first' => [fn () => new FbFormat(FB_LAYOUT_RGB565), 1, [[0, 0, 0x1234], [0, 1, 0xF800]], '1234f800'],
    'rgb565, low byte first' => [fn () => new FbFormat(FB_LAYOUT_RGB565, byteOrder: FB_BYTE_ORDER_LSB), 1, [[0, 0, 0x1234]], '34120000'],
    'bgr565' => [fn () => new FbFormat(FB_LAYOUT_RGB565, channelOrder: FB_CHANNELS_BGR), 1, [[0, 0, 0xF800], [0, 1, 0x07E0]], '001f07e0'],
    'rgb666 keeps six bits a channel' => [fn () => new FbFormat(FB_LAYOUT_RGB666), 1, [[0, 0, 0xFC8004]], 'fc8004000000'],
    'bgr888' => [fn () => new FbFormat(FB_LAYOUT_RGB888, channelOrder: FB_CHANNELS_BGR), 1, [[0, 0, 0x112233]], '332211000000'],
    'rgba8888' => [fn () => new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA), 1, [[0, 0, 0x11223344]], '1122334400000000'],
    'bgra8888' => [fn () => new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_BGRA), 1, [[0, 0, 0x11223344]], '3322114400000000'],
    'argb8888' => [fn () => new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_ARGB), 1, [[0, 0, 0x11223344]], '4411223300000000'],
    'abgr8888' => [fn () => new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_ABGR), 1, [[0, 0, 0x11223344]], '4433221100000000'],
    'planar: a plane per ink, black inverted' => [fn () => bwr(), 8, [[0, 0, 1], [7, 0, 2], [3, 1, 3]], '7fef0110'],
]);

it('masks a word to what the layout stores', function (): void {
    $mono = buffer(FB_LAYOUT_MONO_ROWS, 8, 1);
    $mono->set(0, 0, 3);
    $rgb666 = buffer(FB_LAYOUT_RGB666, 1, 1);
    $rgb666->set(0, 0, 0xFFFFFFFF);
    $index = buffer(FB_LAYOUT_INDEX2, 4, 1);
    $index->set(1, 0, -1);

    expect($mono->get(0, 0))->toBe(1)
        ->and($rgb666->get(0, 0))->toBe(0xFCFCFC)
        ->and(bin2hex($index->bytes()))->toBe('30');
});

it('writes a list whole or not at all, and answers its bounding box', function (): void {
    $buffer = buffer(FB_LAYOUT_INDEX8, 8, 4);

    expect($buffer->setPixels([[1, 1, 9], [5, 3, 7], [2, 0, 1]]))->toBe([1, 0, 5, 4])
        ->and($buffer->get(5, 3))->toBe(7)
        ->and($buffer->setCoordinates([[7, 3], [6, 3]], 4))->toBe([6, 3, 2, 1])
        ->and($buffer->get(6, 3))->toBe(4)
        ->and($buffer->setPixels([]))->toBeNull()
        ->and($buffer->setCoordinates([], 1))->toBeNull();

    $before = $buffer->bytes();

    expect(fn () => $buffer->setPixels([[0, 0, 5], [8, 0, 5]]))->toThrow(ValueError::class, '(8, 0) is outside a 8x4 framebuffer.')
        ->and(fn () => $buffer->setPixels([[0, 0, 5], [0, 0]]))->toThrow(ValueError::class, 'Pixel 1 is not [x, y, value].')
        ->and(fn () => $buffer->setPixels([[0, 0, 5], 'x']))->toThrow(ValueError::class, 'Pixel 1 is not [x, y, value].')
        ->and(fn () => $buffer->setCoordinates([[0, 0], [0, 4]], 5))->toThrow(ValueError::class, '(0, 4) is outside')
        ->and(fn () => $buffer->setCoordinates([[0, 0], [0, '1']], 5))->toThrow(ValueError::class, 'Pixel 1 is not [x, y].')
        ->and($buffer->bytes())->toBe($before);
});

it('fills rects inside the buffer and refuses the rest', function (): void {
    $buffer = buffer(FB_LAYOUT_MONO_ROWS, 8, 2);
    $buffer->rect(2, 1, 4, 1, 1);

    expect(bin2hex($buffer->bytes()))->toBe('003c');

    $buffer->fill(1);

    expect(bin2hex($buffer->bytes()))->toBe('ffff')
        ->and(fn () => $buffer->rect(6, 0, 3, 1, 1))->toThrow(ValueError::class, 'is empty or not inside a 8x2 framebuffer')
        ->and(fn () => $buffer->rect(0, 0, 0, 1, 1))->toThrow(ValueError::class)
        ->and(fn () => $buffer->rect(-1, 0, 1, 1, 1))->toThrow(ValueError::class)
        ->and(fn () => $buffer->get(0, 2))->toThrow(ValueError::class, '(0, 2) is outside a 8x2 framebuffer.')
        ->and(fn () => $buffer->set(8, 0, 1))->toThrow(ValueError::class);
});

it('leaves the padding bits of a filled row alone', function (): void {
    $mono = buffer(FB_LAYOUT_MONO_ROWS, 9, 1);
    $mono->fill(1);
    $rgb444 = buffer(FB_LAYOUT_RGB444, 3, 1);
    $rgb444->fill(0xFFF);

    expect(bin2hex($mono->bytes()))->toBe('ff80')
        ->and(bin2hex($rgb444->bytes()))->toBe('fffffffff000');
});

it('answers a region as stored, or mapped into another format', function (): void {
    $buffer = buffer(FB_LAYOUT_RGB888, 3, 2);
    $buffer->setPixels([[0, 0, 0xFF0000], [1, 0, 0x00FF00], [2, 0, 0x0000FF], [0, 1, 0xFFFFFF], [2, 1, 0x808080]]);

    expect(bin2hex($buffer->region(1, 0, 2, 2)))->toBe('00ff00'.'0000ff'.'000000'.'808080')
        ->and(bin2hex($buffer->region(0, 0, 3, 2, new FbFormat(FB_LAYOUT_RGB565))))->toBe('f800'.'07e0'.'001f'.'ffff'.'0000'.'8410')
        ->and(bin2hex($buffer->region(0, 0, 3, 2, new FbFormat(FB_LAYOUT_MONO_ROWS))))->toBe('40'.'a0')
        ->and(bin2hex($buffer->region(0, 0, 3, 2, new FbFormat(FB_LAYOUT_RGB888, scan: FB_SCAN_BOTTOM_UP))))->toBe('ffffff'.'000000'.'808080'.'ff0000'.'00ff00'.'0000ff')
        ->and(bin2hex($buffer->region(0, 0, 1, 1, new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_BGRA))))->toBe('0000ffff')
        ->and(bin2hex($buffer->rgba8()))->toBe('ff0000ff'.'00ff00ff'.'0000ffff'.'ffffffff'.'000000ff'.'808080ff')
        ->and(fn () => $buffer->region(2, 0, 2, 1))->toThrow(ValueError::class, 'is empty or not inside a 3x2 framebuffer');
});

it('emits its own rows bottom-up when its format scans that way', function (): void {
    $buffer = new FbBuffer(new FbFormat(FB_LAYOUT_INDEX8, scan: FB_SCAN_BOTTOM_UP), 2, 2);
    $buffer->setPixels([[0, 0, 1], [1, 0, 2], [0, 1, 3], [1, 1, 4]]);

    expect(bin2hex($buffer->bytes()))->toBe('01020304')
        ->and(bin2hex($buffer->region(0, 0, 2, 2)))->toBe('03040102');
});

it('maps RGBA8 pixels in, clipped to the buffer and to a clip rect', function (): void {
    $buffer = buffer(FB_LAYOUT_MONO_ROWS, 8, 2);
    $white = str_repeat("\xff", 4 * 2 * 4);

    expect($buffer->blitRgba8($white, 4, 2, 6, 1))->toBe([6, 1, 2, 1])
        ->and(bin2hex($buffer->bytes()))->toBe('0003')
        ->and($buffer->blitRgba8($white, 4, 2, -2, -1))->toBe([0, 0, 2, 1])
        ->and(bin2hex($buffer->bytes()))->toBe('c003')
        ->and($buffer->blitRgba8($white, 4, 2, 2, 0, 3, 0, 2, 1))->toBe([3, 0, 2, 1])
        ->and(bin2hex($buffer->bytes()))->toBe('d803')
        ->and($buffer->blitRgba8($white, 4, 2, 8, 0))->toBeNull()
        ->and($buffer->blitRgba8($white, 4, 2, 0, 0, 0, 0, 0, 2))->toBeNull()
        ->and($buffer->blitRgba8($white, 4, 2, 0, 0, 6, 0, null, 1))->toBeNull()
        ->and(fn () => $buffer->blitRgba8('short', 4, 2, 0, 0))->toThrow(ValueError::class, 'must be 4x2 RGBA8 pixels (32 bytes), 5 given')
        ->and(fn () => $buffer->blitRgba8('', 0, 2, 0, 0))->toThrow(ValueError::class, 'must be between 1 and 65535');
});

it('reads the source pixel that lands on each target pixel', function (): void {
    $buffer = buffer(FB_LAYOUT_RGB888, 2, 2);
    $source = hex2bin('010101ff'.'020202ff'.'030303ff'.'040404ff'.'050505ff'.'060606ff');   // 3x2

    $buffer->blitRgba8($source, 3, 2, -1, 1);

    expect(bin2hex($buffer->bytes()))->toBe('000000'.'000000'.'020202'.'030303');
});

it('copies a rect from a buffer of the same size and storage', function (): void {
    $format = new FbFormat(FB_LAYOUT_INDEX4);
    $source = new FbBuffer($format, 4, 2);
    $source->fill(0xA);
    $target = new FbBuffer(new FbFormat(FB_LAYOUT_INDEX4, scan: FB_SCAN_BOTTOM_UP), 4, 2);

    $target->copy($source, 1, 0, 2, 2);
    expect(bin2hex($target->bytes()))->toBe('0aa00aa0');

    $target->copy($source, 0, 0, 4, 2);
    expect(bin2hex($target->bytes()))->toBe('aaaaaaaa')
        ->and(fn () => $target->copy(new FbBuffer($format, 4, 3), 0, 0, 1, 1))->toThrow(ValueError::class, 'must be a buffer of the same size and storage')
        ->and(fn () => $target->copy(buffer(FB_LAYOUT_INDEX8, 4, 2), 0, 0, 1, 1))->toThrow(ValueError::class)
        ->and(fn () => $target->copy($source, 3, 0, 2, 1))->toThrow(ValueError::class, 'is empty or not inside');
});

it('answers one plane of a planar buffer, and a plane of matching words of any buffer', function (): void {
    $planar = new FbBuffer(bwr(), 8, 1);
    $planar->set(0, 0, 1);
    $planar->set(7, 0, 2);
    $index = buffer(FB_LAYOUT_INDEX4, 9, 1);
    $index->setPixels([[0, 0, 3], [8, 0, 3], [4, 0, 2]]);

    expect(bin2hex($planar->layer(0)))->toBe('7f')
        ->and(bin2hex($planar->layer(1)))->toBe('01')
        ->and(fn () => $planar->layer(2))->toThrow(ValueError::class, 'Layer 2 is outside 0..1.')
        ->and($index->layer(0))->toBe($index->bytes())
        ->and(fn () => $index->layer(1))->toThrow(ValueError::class, 'Layer 1 is outside 0..0.')
        ->and(bin2hex($index->plane(3)))->toBe('8080')
        ->and(bin2hex($index->plane(2)))->toBe('0800')
        ->and(bin2hex($index->plane(-1)))->toBe('0000')
        ->and(bin2hex($index->plane(PHP_INT_MAX)))->toBe('0000');
});

it('refuses a size outside 1..65535', function (int $width, int $height): void {
    expect(fn () => buffer(FB_LAYOUT_MONO_ROWS, $width, $height))->toThrow(ValueError::class, 'must be between 1 and 65535');
})->with([[0, 1], [1, 0], [65536, 1], [1, -5]]);

it('holds the largest side it accepts', function (): void {
    $buffer = buffer(FB_LAYOUT_MONO_ROWS, 65535, 2);
    $buffer->set(65534, 1, 1);

    expect($buffer->size())->toBe(16384)
        ->and($buffer->get(65534, 1))->toBe(1)
        ->and(bin2hex(substr($buffer->bytes(), -1)))->toBe('02');
});

/** Span bytes from [y, x, length, coverage] lists. */
function spanBytes(array ...$spans): string
{
    return implode('', array_map(fn (array $span): string => pack('vvvC', ...$span), $spans));
}

it('paints spans: full coverage writes, partial coverage blends source-over, zero leaves the pixel', function (): void {
    $buffer = buffer(FB_LAYOUT_RGBA8888, 3, 1);
    $buffer->fill(0xFFFFFFFF);

    expect($buffer->paintSpans(spanBytes([0, 0, 1, 255], [0, 1, 1, 128], [0, 2, 1, 0]), 0xFF0000FF))->toBe([0, 0, 3, 1])
        ->and(bin2hex($buffer->bytes()))->toBe('ff0000ff'.'ff7f7fff'.'ffffffff')
        ->and($buffer->paintSpans(spanBytes([0, 2, 1, 255]), 0x0000FF80))->toBe([2, 0, 1, 1])
        ->and(bin2hex($buffer->bytes()))->toBe('ff0000ff'.'ff7f7fff'.'7f7fffff');
});

it('thresholds spans on layouts that cannot blend', function (): void {
    $mono = buffer(FB_LAYOUT_MONO_ROWS, 8, 1);
    $mono->paintSpans(spanBytes([0, 0, 4, 127], [0, 4, 4, 128]), 0xFFFFFFFF);
    $planar = new FbBuffer(bwr(), 8, 1);
    $planar->paintSpans(spanBytes([0, 0, 2, 200], [0, 6, 2, 100]), 0xFF0000FF);

    expect(bin2hex($mono->bytes()))->toBe('0f')
        ->and(bin2hex($planar->bytes()))->toBe('ffc0');
});

it('checks every span before painting any, and answers null for none', function (): void {
    $buffer = buffer(FB_LAYOUT_RGB565, 8, 4);

    expect(fn () => $buffer->paintSpans(spanBytes([0, 0, 2, 255], [1, 5, 4, 255]), 0xFFFFFFFF))->toThrow(ValueError::class, 'Span 1 is empty or not inside a 8x4 framebuffer.')
        ->and(fn () => $buffer->paintSpans(spanBytes([0, 0, 2, 255], [4, 0, 1, 255]), 0xFFFFFFFF))->toThrow(ValueError::class, 'Span 1 is empty')
        ->and(fn () => $buffer->paintSpans(spanBytes([0, 0, 2, 255], [1, 1, 0, 255]), 0xFFFFFFFF))->toThrow(ValueError::class, 'Span 1 is empty')
        ->and(fn () => $buffer->paintSpans(spanBytes([0, 0, 1, 255]).'xyz', 0xFFFFFFFF))->toThrow(ValueError::class, 'A span list is a whole number of 7-byte spans, got 10 bytes.')
        ->and(fn () => $buffer->paintSpans('', -1))->toThrow(ValueError::class, 'paintSpans() takes a colour 0xRRGGBBAA, got -1.')
        ->and(fn () => $buffer->paintSpans('', 0x100000000))->toThrow(ValueError::class, 'takes a colour 0xRRGGBBAA')
        ->and(bin2hex($buffer->bytes()))->toBe(str_repeat('00', 64))
        ->and($buffer->paintSpans('', 0xFFFFFFFF))->toBeNull()
        ->and($buffer->paintSpans(spanBytes([3, 6, 2, 1]), 0))->toBe([6, 3, 2, 1]);
});

it('paints RGBA8 pixels through an inverse placement: nearest, smooth, turned, blended', function (): void {
    $red = 'ff0000ff';
    $green = '00ff00ff';
    $scaled = buffer(FB_LAYOUT_RGBA8888, 4, 2);
    $scaled->paintRgba8(hex2bin($red.$green), 2, 1, [0.5, 0, 0, 0.5, 0, 0], 0, 0, 4, 2);
    $smooth = buffer(FB_LAYOUT_RGBA8888, 4, 1);
    $smooth->paintRgba8(hex2bin('000000ff'.'ffffffff'), 2, 1, [0.5, 0, 0, 1, 0, 0], 0, 0, 4, 1, 255, true);
    $clear = buffer(FB_LAYOUT_RGBA8888, 4, 1);
    $clear->paintRgba8(hex2bin('00ff0000'.'ff0000ff'), 2, 1, [0.5, 0, 0, 1, 0, 0], 0, 0, 4, 1, 255, true);
    $turned = buffer(FB_LAYOUT_RGBA8888, 2, 2);
    $turned->paintRgba8(hex2bin($red.$green), 2, 1, [0, -1, 1, 0, 0, 2], 0, 0, 2, 2);
    $blended = buffer(FB_LAYOUT_RGBA8888, 2, 1);
    $blended->fill(0xFFFFFFFF);
    $blended->paintRgba8(hex2bin($red), 1, 1, [1, 0, 0, 1, 0, 0], 0, 0, 1, 1, 128);
    $blended->paintRgba8(hex2bin('ff000080'), 1, 1, [1, 0, 0, 1, -1, 0], 1, 0, 1, 1);
    $mono = buffer(FB_LAYOUT_MONO_ROWS, 8, 1);
    $mono->paintRgba8(hex2bin('ffffff7f'.'ffffff80'), 2, 1, [1, 0, 0, 1, 0, 0], 0, 0, 8, 1);

    expect(bin2hex($scaled->bytes()))->toBe($red.$red.$green.$green.$red.$red.$green.$green)
        ->and(bin2hex($smooth->bytes()))->toBe('000000ff'.'404040ff'.'bfbfbfff'.'ffffffff')
        ->and(bin2hex($clear->bytes()))->toBe('00000000'.'40000040'.'bf0000bf'.'ff0000ff')
        ->and(bin2hex($turned->bytes()))->toBe('00000000'.$red.'00000000'.$green)
        ->and(bin2hex($blended->bytes()))->toBe('ff7f7fff'.'ff7f7fff')
        ->and(bin2hex($mono->bytes()))->toBe('40');
});

it('counts rows from the surface row its own row 0 stands for', function (): void {
    $window = buffer(FB_LAYOUT_RGBA8888, 1, 2);
    $window->paintRgba8(hex2bin('ff0000ff'.'00ff00ff'.'0000ffff'.'ffffffff'), 1, 4, [1, 0, 0, 1, 0, 0], 0, 0, 1, 2, 255, false, 2);

    expect(bin2hex($window->bytes()))->toBe('0000ffff'.'ffffffff');
});

it('refuses a malformed image, inverse, target, opacity or row, painting nothing', function (): void {
    $buffer = buffer(FB_LAYOUT_RGB565, 4, 4);
    $pixel = "\xff\xff\xff\xff";
    $identity = [1, 0, 0, 1, 0, 0];

    expect(fn () => $buffer->paintRgba8('short', 1, 1, $identity, 0, 0, 1, 1))->toThrow(ValueError::class, 'must be 1x1 RGBA8 pixels (4 bytes), 5 given')
        ->and(fn () => $buffer->paintRgba8('', 0, 1, $identity, 0, 0, 1, 1))->toThrow(ValueError::class, 'must be between 1 and 65535')
        ->and(fn () => $buffer->paintRgba8($pixel, 1, 1, [1, 0, 0, 1, 0], 0, 0, 1, 1))->toThrow(ValueError::class, 'must be a list of six finite numbers')
        ->and(fn () => $buffer->paintRgba8($pixel, 1, 1, [1, 0, 0, 1, 0, NAN], 0, 0, 1, 1))->toThrow(ValueError::class, 'must be a list of six finite numbers')
        ->and(fn () => $buffer->paintRgba8($pixel, 1, 1, [1, 0, 0, 1, 0, '0'], 0, 0, 1, 1))->toThrow(ValueError::class, 'must be a list of six finite numbers')
        ->and(fn () => $buffer->paintRgba8($pixel, 1, 1, ['a' => 1, 0, 0, 1, 0, 0], 0, 0, 1, 1))->toThrow(ValueError::class, 'must be a list of six finite numbers')
        ->and(fn () => $buffer->paintRgba8($pixel, 1, 1, $identity, 3, 0, 2, 1))->toThrow(ValueError::class, 'is empty or not inside a 4x4 framebuffer')
        ->and(fn () => $buffer->paintRgba8($pixel, 1, 1, $identity, 0, 0, 0, 1))->toThrow(ValueError::class, 'is empty or not inside')
        ->and(fn () => $buffer->paintRgba8($pixel, 1, 1, $identity, 0, 0, 1, 1, 256))->toThrow(ValueError::class, 'must be between 0 and 255')
        ->and(fn () => $buffer->paintRgba8($pixel, 1, 1, $identity, 0, 0, 1, 1, -1))->toThrow(ValueError::class, 'must be between 0 and 255')
        ->and(fn () => $buffer->paintRgba8($pixel, 1, 1, $identity, 0, 0, 1, 1, 255, false, -1))->toThrow(ValueError::class, 'must be between 0 and 65535')
        ->and(fn () => $buffer->paintRgba8($pixel, 1, 1, $identity, 0, 0, 1, 1, 255, false, 65536))->toThrow(ValueError::class, 'must be between 0 and 65535')
        ->and(bin2hex($buffer->bytes()))->toBe(str_repeat('00', 32));
});
