<?php

declare(strict_types=1);

// FbBuffer::over() with a stride: a buffer over padded rows, as a library that aligns its rows keeps them
// (pixman pads every row to 4 bytes). Held to a packed buffer given the same calls: the same pixels, and the
// padding between rows never written.

dataset('stride formats', [
    'rgba8' => [fn () => new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA), 4],
    'bgra8' => [fn () => new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_BGRA), 4],
    'rgb888' => [fn () => new FbFormat(FB_LAYOUT_RGB888), 3],
    'rgb565' => [fn () => new FbFormat(FB_LAYOUT_RGB565), 2],
    'grey8' => [fn () => new FbFormat(FB_LAYOUT_INDEX8), 1],
]);

const SW = 13;
const SH = 7;
const PAD = 9;      // bytes after each row: not a multiple of any pixel size
const SENTINEL = 0xA5;

/**
 * A packed buffer and a buffer over padded rows, both holding the same random pixels. The padded one's owner is a
 * grey index8 buffer one stride wide, whose padding bytes are filled with SENTINEL.
 *
 * @return array{FbBuffer, FbBuffer, FbBuffer, int}
 */
function stride_pair(FbFormat $format, int $bytes): array
{
    $stride = SW * $bytes + PAD;
    $owner = new FbBuffer(new FbFormat(FB_LAYOUT_INDEX8), $stride, SH);
    $owner->rect(0, 0, $stride, SH, SENTINEL);
    $padded = FbBuffer::over($format, SW, SH, $owner->pointer(), $owner, $stride);
    $packed = new FbBuffer($format, SW, SH);
    $rgba = random_bytes(SW * SH * 4);
    $padded->blitRgba8($rgba, SW, SH, 0, 0);
    $packed->blitRgba8($rgba, SW, SH, 0, 0);

    return [$packed, $padded, $owner, $stride];
}

/** The padding bytes of every row, as the owner holds them. */
function stride_padding(FbBuffer $owner, int $stride, int $bytes): string
{
    $out = '';
    for ($y = 0; $y < SH; $y++) {
        $out .= substr($owner->bytes(), $y * $stride + SW * $bytes, PAD);
    }

    return $out;
}

function stride_same(FbBuffer $packed, FbBuffer $padded, FbBuffer $owner, int $stride, int $bytes): void
{
    expect($padded->rgba8())->toBe($packed->rgba8())
        ->and($padded->region(0, 0, SW, SH))->toBe($packed->region(0, 0, SW, SH))
        ->and($padded->region(3, 2, 7, 4))->toBe($packed->region(3, 2, 7, 4))
        ->and(stride_padding($owner, $stride, $bytes))->toBe(str_repeat(chr(SENTINEL), PAD * SH));
}

beforeEach(fn () => mt_srand(23));

it('holds and reads pixels over padded rows as over packed ones', function (Closure $format, int $bytes): void {
    [$packed, $padded, $owner, $stride] = stride_pair($format(), $bytes);
    expect($padded->size())->toBe($stride * SH);
    stride_same($packed, $padded, $owner, $stride, $bytes);

    foreach ([[0, 0], [SW - 1, 0], [5, 3], [SW - 1, SH - 1]] as [$x, $y]) {
        expect($padded->get($x, $y))->toBe($packed->get($x, $y));
    }
    [$word, $other] = [$packed->get(2, 2), $packed->get(0, 0)];
    foreach ([$packed, $padded] as $b) {
        $b->set(SW - 1, SH - 1, $word);
        $b->rect(2, 1, 9, 4, $word);
    }
    stride_same($packed, $padded, $owner, $stride, $bytes);
    foreach ([$packed, $padded] as $b) {
        $b->fill($other);
    }
    stride_same($packed, $padded, $owner, $stride, $bytes);
})->with('stride formats');

it('paints spans, masks, gradients and images over padded rows as over packed ones', function (Closure $format, int $bytes): void {
    [$packed, $padded, $owner, $stride] = stride_pair($format(), $bytes);
    $spans = pack('vvvC', 1, 0, SW, 200).pack('vvvC', 3, 2, 9, 77).pack('vvvC', SH - 1, 5, 8, 255);
    $masks = [[random_bytes(6 * 4), 6, 4, 1], [random_bytes(SW * 2), SW, 0, SH - 2]];
    $ramp = random_bytes(1024);
    $source = random_bytes(5 * 4 * 4);
    foreach ([$packed, $padded] as $b) {
        $b->paintSpans($spans, 0x80FF4020);
        $b->paintMasks($masks, 0xC0204080, 0, 0, SW, SH);
        $b->paintGradient($spans, $ramp, false, 0, [0.1, 0.0, 0.0, 0.1, 0.0, 0.0]);
        $b->paintRgba8($source, 5, 4, [0.5, 0.1, -0.1, 0.5, 0.2, 0.3], 0, 0, SW, SH, 220, true);
    }
    stride_same($packed, $padded, $owner, $stride, $bytes);
})->with('stride formats');

it('blits between padded and packed rows, and within its own padded rows', function (Closure $format, int $bytes): void {
    [$packed, $padded, $owner, $stride] = stride_pair($format(), $bytes);
    $other = new FbBuffer($format(), 6, 5);
    $other->blitRgba8(random_bytes(6 * 5 * 4), 6, 5, 0, 0);
    foreach ([$packed, $padded] as $b) {
        $b->blit($other, 1, 1, 5, 4, 7, 2);         // from packed rows
        $b->blit($b, 0, 0, 8, 5, 3, 1);             // within itself, overlapping
    }
    stride_same($packed, $padded, $owner, $stride, $bytes);

    $into = new FbBuffer($format(), SW, SH);
    $into->blit($padded, 0, 0, SW, SH, 0, 0);       // from padded rows
    expect($into->rgba8())->toBe($packed->rgba8());

    $painted = [new FbBuffer($format(), 9, 9), new FbBuffer($format(), 9, 9)];
    $painted[0]->paintBuffer($packed, [0.7, 0.2, -0.2, 0.7, 1.0, 0.5], 0, 0, 9, 9, 200, true);
    $painted[1]->paintBuffer($padded, [0.7, 0.2, -0.2, 0.7, 1.0, 0.5], 0, 0, 9, 9, 200, true);
    expect($painted[1]->rgba8())->toBe($painted[0]->rgba8());
})->with('stride formats');

it('refuses a stride shorter than a packed row, padded rows for packed-bit formats, and effects over padded rows', function (): void {
    $rgba = new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA);
    $owner = new FbBuffer(new FbFormat(FB_LAYOUT_INDEX8), 64, 8);
    expect(fn () => FbBuffer::over($rgba, 13, 8, $owner->pointer(), $owner, 51))->toThrow(ValueError::class)
        ->and(fn () => FbBuffer::over(new FbFormat(FB_LAYOUT_MONO_ROWS), 13, 8, $owner->pointer(), $owner, 4))->toThrow(ValueError::class);

    $padded = FbBuffer::over($rgba, 13, 8, $owner->pointer(), $owner, 56);
    $packed = new FbBuffer($rgba, 13, 8);
    expect(fn () => $padded->kawaseUp($packed))->toThrow(ValueError::class)
        ->and(fn () => $packed->kawaseUp($padded))->toThrow(ValueError::class)
        ->and(fn () => $padded->copy($packed, 0, 0, 13, 8))->toThrow(ValueError::class);

    // A stride equal to a packed row is packed.
    $exact = FbBuffer::over($rgba, 13, 8, $owner->pointer(), $owner, 52);
    expect($exact->size())->toBe(13 * 8 * 4);
});
