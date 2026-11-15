<?php

declare(strict_types=1);

// FbBuffer::blit() and paintBuffer(): a buffer as the source, held to the string paths (blitRgba8() and
// paintRgba8() fed the source's rgba8()), which must give the same bytes.

dataset('blit formats', [
    'rgba8' => [fn () => new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA)],
    'bgra8' => [fn () => new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_BGRA)],
    'rgb888' => [fn () => new FbFormat(FB_LAYOUT_RGB888)],
    'rgb565' => [fn () => new FbFormat(FB_LAYOUT_RGB565)],
    'mono' => [fn () => new FbFormat(FB_LAYOUT_MONO_ROWS)],
    'bottom-up rgba8' => [fn () => new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA, scan: FB_SCAN_BOTTOM_UP)],
]);

function blit_random(FbFormat $format, int $width, int $height): FbBuffer
{
    $buffer = new FbBuffer($format, $width, $height);
    $buffer->blitRgba8(random_bytes($width * $height * 4), $width, $height, 0, 0);

    return $buffer;
}

beforeEach(fn () => mt_srand(5));

it('copies a rect between any two formats as the RGBA8 string path does', function (Closure $from, Closure $to) {
    $source = blit_random($from(), 13, 9);
    $a = blit_random($to(), 17, 11);
    $b = new FbBuffer($to(), 17, 11);
    $b->blitRgba8($a->rgba8(), 17, 11, 0, 0);
    expect($b->bytes())->toBe($a->bytes());

    $landed = $a->blit($source, 2, 3, 8, 5, 6, 4);
    // the reference: the same rect of the source as RGBA8, landed by blitRgba8()
    $rect = '';
    $all = $source->rgba8();
    for ($y = 3; $y < 8; $y++) {
        $rect .= substr($all, ($y * 13 + 2) * 4, 8 * 4);
    }
    $b->blitRgba8($rect, 8, 5, 6, 4);

    expect($landed)->toBe([6, 4, 8, 5])
        ->and($a->bytes())->toBe($b->bytes());
})->with('blit formats')->with('blit formats');

it('copies words as stored when both share storage', function () {
    $format = new FbFormat(FB_LAYOUT_RGB565);
    $source = new FbBuffer($format, 4, 1);
    foreach ([0x1234, 0xABCD, 0x0F0F, 0xFFFF] as $x => $word) {
        $source->set($x, 0, $word);
    }
    $target = new FbBuffer($format, 4, 1);
    $target->blit($source, 0, 0, 4, 1, 0, 0);

    expect($target->bytes())->toBe($source->bytes());
});

it('clips to the buffer and to the clip rect, as blitRgba8() clips', function () {
    $format = new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA);
    $source = blit_random($format, 10, 10);
    $a = new FbBuffer($format, 8, 8);
    $b = new FbBuffer($format, 8, 8);

    $landed = $a->blit($source, 0, 0, 10, 10, -2, 3, 1, 0, 5, null);
    $expected = $b->blitRgba8($source->rgba8(), 10, 10, -2, 3, 1, 0, 5, null);

    expect($landed)->toBe($expected)
        ->and($a->bytes())->toBe($b->bytes())
        ->and($a->blit($source, 0, 0, 3, 3, 20, 20))->toBeNull();
});

it('refuses a source rect outside the source', function () {
    $format = new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA);
    (new FbBuffer($format, 4, 4))->blit(new FbBuffer($format, 4, 4), 2, 2, 3, 1, 0, 0);
})->throws(ValueError::class);

it('copies within one buffer, overlap included, as a copy through a second buffer would', function (Closure $format, int $dx, int $dy) {
    $a = blit_random($format(), 16, 12);
    $b = new FbBuffer($format(), 16, 12);
    $b->blit($a, 0, 0, 16, 12, 0, 0);
    $held = new FbBuffer($format(), 16, 12);
    $held->blit($a, 0, 0, 16, 12, 0, 0);

    $a->blit($a, 3, 2, 9, 7, 3 + $dx, 2 + $dy);
    $b->blit($held, 3, 2, 9, 7, 3 + $dx, 2 + $dy);

    expect($a->bytes())->toBe($b->bytes());
})->with('blit formats')->with([[2, 1], [-2, -1], [3, 0], [-3, 0], [0, 2], [0, -2]]);

it('copies from a buffer over another owner\'s bytes', function () {
    $format = new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_BGRA);
    $owner = blit_random($format, 6, 4);
    $over = FbBuffer::over($format, 6, 4, $owner->pointer(), $owner);
    $target = new FbBuffer(new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA), 6, 4);
    $target->blit($over, 0, 0, 6, 4, 0, 0);

    expect($target->rgba8())->toBe($owner->rgba8());
});

it('paints a buffer as paintRgba8() paints its RGBA8', function (Closure $from, Closure $to) {
    $source = blit_random($from(), 9, 7);
    $a = blit_random($to(), 24, 20);
    $b = new FbBuffer($to(), 24, 20);
    $b->blitRgba8($a->rgba8(), 24, 20, 0, 0);
    $inverse = [0.55, 0.12, -0.1, 0.6, -1.3, 0.4];
    $coverage = pack('vvvC', 3, 2, 10, 200).pack('vvvC', 4, 5, 6, 90);

    $a->paintBuffer($source, $inverse, 1, 1, 20, 16, 210, true, 0, 0xFFC080FF, [0.5, 0.5, 8.0, 6.5], null);
    $b->paintRgba8($source->rgba8(), 9, 7, $inverse, 1, 1, 20, 16, 210, true, 0, 0xFFC080FF, [0.5, 0.5, 8.0, 6.5], null);
    expect($a->bytes())->toBe($b->bytes());

    $a->paintBuffer($source, $inverse, 0, 0, 24, 20, 255, false, 0, 0xFFFFFFFF, null, $coverage);
    $b->paintRgba8($source->rgba8(), 9, 7, $inverse, 0, 0, 24, 20, 255, false, 0, 0xFFFFFFFF, null, $coverage);
    expect($a->bytes())->toBe($b->bytes());
})->with('blit formats')->with([
    'into rgba8' => [fn () => new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA)],
    'into rgb565' => [fn () => new FbFormat(FB_LAYOUT_RGB565)],
]);

it('reports paintBuffer() argument errors by paintBuffer()\'s own positions', function () {
    $format = new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA);
    (new FbBuffer($format, 4, 4))->paintBuffer(new FbBuffer($format, 2, 2), [1, 0, 0, 1, 0, 0], 0, 0, 4, 4, 300);
})->throws(ValueError::class, 'paintBuffer(): Argument #7 ($opacity) must be between 0 and 255');
