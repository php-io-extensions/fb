<?php

declare(strict_types=1);

/*
 * coverage(): a buffer read as coverage, as spans, in C. A library that draws coverage into a
 * buffer by address gets its spans without a byte of it passing through PHP.
 */

/** @return list<array{int, int, int, int}> each span as [y, x, length, coverage] */
function coverageSpans(string $spans): array
{
    return array_map(fn (string $s): array => array_values(unpack('vy/vx/vlength/Ccoverage', $s)), $spans === '' ? [] : str_split($spans, 7));
}

it('reads an INDEX8 buffer run by run, a run of one coverage one span, zero skipped, placed at the offset', function () {
    $b = buffer(FB_LAYOUT_INDEX8, 6, 2);
    $b->rect(1, 0, 3, 1, 200);
    $b->rect(4, 0, 1, 1, 40);
    $b->rect(0, 1, 6, 1, 255);

    expect(coverageSpans($b->coverage(10, 20)))->toBe([[20, 11, 3, 200], [20, 14, 1, 40], [21, 10, 6, 255]])
        ->and(coverageSpans($b->coverage()))->toBe([[0, 1, 3, 200], [0, 4, 1, 40], [1, 0, 6, 255]]);
});

it('reads the alpha of a 32-bit buffer, wherever its channel order puts it', function (int $order) {
    $b = new FbBuffer(new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: $order), 4, 1);
    $b->blitRgba8("\x10\x20\x30\x00"."\x10\x20\x30\x80"."\xff\x00\x00\x80"."\x00\x00\x00\xff", 4, 1, 0, 0);

    // Colour does not split a run: pixels 1 and 2 share alpha 128.
    expect(coverageSpans($b->coverage()))->toBe([[0, 1, 2, 128], [0, 3, 1, 255]]);
})->with([
    'rgba' => [FB_CHANNELS_RGBA],
    'bgra' => [FB_CHANNELS_BGRA],
    'argb' => [FB_CHANNELS_ARGB],
    'abgr' => [FB_CHANNELS_ABGR],
]);

it('reads rows as get() does, a bottom-up buffer too, and through a padded stride', function () {
    $up = new FbBuffer(new FbFormat(FB_LAYOUT_INDEX8, scan: FB_SCAN_BOTTOM_UP), 3, 2);
    $up->rect(0, 0, 3, 1, 9);
    $owner = buffer(FB_LAYOUT_INDEX8, 8, 2);
    $owner->rect(0, 0, 8, 2, 7);
    $owner->rect(0, 1, 3, 1, 50);
    $view = FbBuffer::over(new FbFormat(FB_LAYOUT_INDEX8), 3, 2, $owner->pointer(), $owner, 8);

    expect(coverageSpans($up->coverage()))->toBe([[0, 0, 3, 9]])
        ->and(coverageSpans($view->coverage()))->toBe([[0, 0, 3, 7], [1, 0, 3, 50]]);
});

it('answers nothing for a buffer of no coverage, and refuses other layouts and offsets past 65535', function () {
    expect(buffer(FB_LAYOUT_INDEX8, 4, 4)->coverage())->toBe('')
        ->and(fn () => buffer(FB_LAYOUT_RGB565, 4, 4)->coverage())->toThrow(ValueError::class, 'INDEX8 or RGBA8888')
        ->and(fn () => buffer(FB_LAYOUT_INDEX8, 4, 4)->coverage(65533, 0))->toThrow(ValueError::class)
        ->and(fn () => buffer(FB_LAYOUT_INDEX8, 4, 4)->coverage(-1, 0))->toThrow(ValueError::class)
        ->and(fn () => buffer(FB_LAYOUT_INDEX8, 4, 4)->coverage(0, 65533))->toThrow(ValueError::class);
});
