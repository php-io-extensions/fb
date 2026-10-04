<?php

declare(strict_types=1);

if (! extension_loaded('fb')) {
    throw new RuntimeException('The fb extension is not loaded; run pest with -d extension=/path/to/fb.so');
}

/** A buffer of one layout with the default orders. */
function buffer(int $layout, int $width, int $height): FbBuffer
{
    return new FbBuffer(new FbFormat($layout, channelOrder: $layout === FB_LAYOUT_RGBA8888 ? FB_CHANNELS_RGBA : FB_CHANNELS_RGB), $width, $height);
}

/** Black/white/red in two planes, the black plane inverted: [rgb, inverted, code]. */
function bwr(): FbFormat
{
    return new FbFormat(FB_LAYOUT_PLANAR, palette: [[0x000000, true, 0], [0xFF0000, false, 1]]);
}
