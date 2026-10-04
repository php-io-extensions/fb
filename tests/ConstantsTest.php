<?php

declare(strict_types=1);

it('numbers the layouts, orders and scans', function (): void {
    expect([FB_LAYOUT_MONO_ROWS, FB_LAYOUT_MONO_PAGES, FB_LAYOUT_INDEX2, FB_LAYOUT_INDEX4, FB_LAYOUT_INDEX8, FB_LAYOUT_RGB444,
        FB_LAYOUT_RGB565, FB_LAYOUT_RGB666, FB_LAYOUT_RGB888, FB_LAYOUT_RGBA8888, FB_LAYOUT_PLANAR])->toBe(range(0, 10))
        ->and([FB_BIT_ORDER_MSB, FB_BIT_ORDER_LSB])->toBe([0, 1])
        ->and([FB_BYTE_ORDER_MSB, FB_BYTE_ORDER_LSB])->toBe([0, 1])
        ->and([FB_CHANNELS_RGB, FB_CHANNELS_BGR, FB_CHANNELS_RGBA, FB_CHANNELS_BGRA, FB_CHANNELS_ARGB, FB_CHANNELS_ABGR])->toBe(range(0, 5))
        ->and([FB_SCAN_TOP_DOWN, FB_SCAN_BOTTOM_UP])->toBe([0, 1]);
});
