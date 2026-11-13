<?php

declare(strict_types=1);

// FbBuffer::over(): a buffer over bytes another object owns. Here the owner is another FbBuffer, which stands in
// for any extension's memory.

it('reads and writes the owner\'s bytes, with nothing copied or blanked', function (): void {
    $format = new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA);
    $owner = new FbBuffer($format, 4, 3);
    $owner->set(1, 1, 0x11223344);

    $over = FbBuffer::over($format, 4, 3, $owner->pointer(), $owner);

    expect($over->pointer())->toBe($owner->pointer())
        ->and($over->width())->toBe(4)
        ->and($over->height())->toBe(3)
        ->and($over->size())->toBe(48)
        ->and($over->format())->toBe($format)
        ->and($over->get(1, 1))->toBe(0x11223344);   // not blanked

    $over->set(2, 0, 0xAABBCCDD);
    $over->paintSpans(pack('vvvC', 2, 0, 4, 255), 0xFF0000FF);

    expect($owner->get(2, 0))->toBe(0xAABBCCDD)
        ->and($owner->get(3, 2))->toBe($format->mapRgba8(255, 0, 0, 255))
        ->and($owner->bytes())->toBe($over->bytes());
});

it('keeps the owner alive, and leaves the bytes to it when it goes', function (): void {
    $format = new FbFormat(FB_LAYOUT_RGB565);
    $owner = new FbBuffer($format, 8, 8);
    $over = FbBuffer::over($format, 8, 8, $owner->pointer(), $owner);
    $weak = WeakReference::create($owner);
    unset($owner);

    expect($weak->get())->not->toBeNull();
    $over->fill(0xFFFF);
    expect($weak->get()->get(7, 7))->toBe(0xFFFF);

    unset($over);
    expect($weak->get())->toBeNull();   // the bytes went with their owner, once
});

it('blends what a buffer of its own blends', function (): void {
    $format = new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA);
    [$own, $owner] = [new FbBuffer($format, 16, 4), new FbBuffer($format, 16, 4)];
    $over = FbBuffer::over($format, 16, 4, $owner->pointer(), $owner);
    $spans = pack('vvvC', 0, 0, 16, 90).pack('vvvC', 1, 3, 9, 200).pack('vvvC', 3, 15, 1, 255);

    foreach ([$own, $over] as $buffer) {
        $buffer->fill(0x204060FF);
        $buffer->paintSpans($spans, 0xE0A01080);
    }

    expect(bin2hex($owner->bytes()))->toBe(bin2hex($own->bytes()));
});

it('refuses a shape a buffer cannot have, and no address', function (): void {
    $format = new FbFormat(FB_LAYOUT_RGBA8888, channelOrder: FB_CHANNELS_RGBA);
    $owner = new FbBuffer($format, 2, 2);

    expect(fn () => FbBuffer::over($format, 0, 2, $owner->pointer(), $owner))->toThrow(ValueError::class, 'must be between 1 and')
        ->and(fn () => FbBuffer::over($format, 2, 70000, $owner->pointer(), $owner))->toThrow(ValueError::class, 'must be between 1 and')
        ->and(fn () => FbBuffer::over($format, 2, 2, 0, $owner))->toThrow(ValueError::class, 'must be the address of the bytes');
});
