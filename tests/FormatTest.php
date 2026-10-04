<?php

declare(strict_types=1);

it('keeps what it was made with', function (): void {
    $format = new FbFormat(FB_LAYOUT_RGB565, FB_BIT_ORDER_LSB, FB_BYTE_ORDER_LSB, FB_CHANNELS_BGR, FB_SCAN_BOTTOM_UP);

    expect($format->layout())->toBe(FB_LAYOUT_RGB565)
        ->and($format->bitOrder())->toBe(FB_BIT_ORDER_LSB)
        ->and($format->byteOrder())->toBe(FB_BYTE_ORDER_LSB)
        ->and($format->channelOrder())->toBe(FB_CHANNELS_BGR)
        ->and($format->scan())->toBe(FB_SCAN_BOTTOM_UP)
        ->and((new FbFormat(FB_LAYOUT_MONO_ROWS))->bitOrder())->toBe(FB_BIT_ORDER_MSB);
});

it('sizes a surface by its layout', function (int $layout, int $bytes): void {
    $format = $layout === FB_LAYOUT_PLANAR ? bwr() : new FbFormat($layout, channelOrder: $layout === FB_LAYOUT_RGBA8888 ? FB_CHANNELS_RGBA : FB_CHANNELS_RGB);

    expect($format->size(9, 10))->toBe($bytes);
})->with([
    'mono rows: two bytes a row' => [FB_LAYOUT_MONO_ROWS, 20],
    'mono pages: a byte per column per 8 rows' => [FB_LAYOUT_MONO_PAGES, 18],
    'index 2' => [FB_LAYOUT_INDEX2, 30],
    'index 4' => [FB_LAYOUT_INDEX4, 50],
    'index 8' => [FB_LAYOUT_INDEX8, 90],
    'rgb444: three bytes a pixel pair' => [FB_LAYOUT_RGB444, 150],
    'rgb565' => [FB_LAYOUT_RGB565, 180],
    'rgb666' => [FB_LAYOUT_RGB666, 270],
    'rgb888' => [FB_LAYOUT_RGB888, 270],
    'rgba8888' => [FB_LAYOUT_RGBA8888, 360],
    'planar: a mono plane per ink' => [FB_LAYOUT_PLANAR, 40],
]);

it('refuses a format that stores nothing', function (Closure $make, string $why): void {
    expect($make)->toThrow(ValueError::class, $why);
})->with([
    'an unknown layout' => [fn () => new FbFormat(11), 'layout must be one of the FB_LAYOUT_* constants'],
    'a negative layout' => [fn () => new FbFormat(-1), 'layout must be'],
    'an unknown bit order' => [fn () => new FbFormat(FB_LAYOUT_MONO_ROWS, 2), 'bitOrder must be'],
    'an unknown byte order' => [fn () => new FbFormat(FB_LAYOUT_RGB565, byteOrder: 2), 'byteOrder must be'],
    'an unknown scan' => [fn () => new FbFormat(FB_LAYOUT_RGB565, scan: 2), 'scan must be'],
    'a three-channel order at 32 bits' => [fn () => new FbFormat(FB_LAYOUT_RGBA8888), 'channelOrder of a 32-bit layout'],
    'an alpha order at 24 bits' => [fn () => new FbFormat(FB_LAYOUT_RGB888, channelOrder: FB_CHANNELS_BGRA), 'channelOrder of a 12 to 24-bit layout'],
    'a channel order on mono' => [fn () => new FbFormat(FB_LAYOUT_MONO_ROWS, channelOrder: FB_CHANNELS_BGR), 'channelOrder applies to the RGB layouts only'],
    'planar without a palette' => [fn () => new FbFormat(FB_LAYOUT_PLANAR), 'a planar layout needs a palette'],
    'a palette on an RGB layout' => [fn () => new FbFormat(FB_LAYOUT_RGB888, palette: [[0, false, 0]]), 'palette applies to the index and planar layouts only'],
    'a palette code past the bits' => [fn () => new FbFormat(FB_LAYOUT_INDEX2, palette: [[0, false, 4]]), 'a palette code must fit'],
    'seventeen inks' => [fn () => new FbFormat(FB_LAYOUT_PLANAR, palette: array_fill(0, 17, [0, false, 0])), 'at most 16 inks'],
    'a malformed ink' => [fn () => new FbFormat(FB_LAYOUT_PLANAR, palette: [[0, 0, 0]]), 'must hold [int $rgb, bool $inverted, int $code] entries'],
    'a colour past 24 bits' => [fn () => new FbFormat(FB_LAYOUT_PLANAR, palette: [[0x1000000, false, 0]]), 'must hold'],
    'a size of zero' => [fn () => (new FbFormat(FB_LAYOUT_RGB888))->size(0, 1), 'must be between 1 and 65535'],
]);

it('maps colours to words and back in integers', function (): void {
    $mono = new FbFormat(FB_LAYOUT_MONO_ROWS);
    $grey = new FbFormat(FB_LAYOUT_INDEX4);
    $rgb565 = new FbFormat(FB_LAYOUT_RGB565);
    $rgba = new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_BGRA);
    $index = new FbFormat(FB_LAYOUT_INDEX4, palette: [[0x000000, false, 0], [0xFFFFFF, false, 1], [0xFF0000, false, 3]]);

    expect($mono->mapRgba8(128, 128, 128))->toBe(1)
        ->and($mono->mapRgba8(127, 127, 127))->toBe(0)
        ->and($mono->unmapRgba8(1))->toBe(0xFFFFFFFF)
        ->and($grey->mapRgba8(85, 85, 85))->toBe(5)
        ->and($grey->unmapRgba8(10))->toBe(0xAAAAAAFF)
        ->and($rgb565->mapRgba8(255, 0, 0))->toBe(0xF800)
        ->and($rgb565->unmapRgba8(0x8410))->toBe(0x848284FF)
        ->and($rgba->mapRgba8(1, 2, 3, 4))->toBe(0x01020304)
        ->and($rgba->unmapRgba8(0x01020304))->toBe(0x01020304)
        ->and($index->mapRgba8(250, 10, 10))->toBe(3)
        ->and($index->unmapRgba8(3))->toBe(0xFF0000FF)
        ->and($index->unmapRgba8(2))->toBe(0xFFFFFFFF)
        ->and(bwr()->mapRgba8(255, 255, 255))->toBe(0)
        ->and(bwr()->mapRgba8(200, 30, 30))->toBe(2)
        ->and(bwr()->unmapRgba8(3))->toBe(0x000000FF)
        ->and(fn () => $mono->mapRgba8(256, 0, 0))->toThrow(ValueError::class, 'must be between 0 and 255');
});
