namespace Fb;

/**
 * Five framebuffer kinds in C, addressed by int handles. Every method is one
 * C call; jovian/fb owns the handle's lifetime and the Surface contract.
 */
class Buffer
{
    public static function create(int kind, int pixelFormat, int bitDepth, int bitOrder, int endianness, int pageAxis, int scan, array palette, int width, int height, int extra) -> int
    {
        return fb_create(kind, pixelFormat, bitDepth, bitOrder, endianness, pageAxis, scan, palette, width, height, extra);
    }

    public static function free(int handle) -> void
    {
        fb_free(handle);
    }

    public static function width(int handle) -> int { return fb_width(handle); }
    public static function height(int handle) -> int { return fb_height(handle); }
    public static function size(int handle) -> int { return fb_size(handle); }
    public static function get(int handle, int x, int y) -> int { return fb_get(handle, x, y); }
    public static function set(int handle, int x, int y, int value) -> void { fb_set(handle, x, y, value); }
    public static function setPixels(int handle, array pixels) -> void { fb_set_pixels(handle, pixels); }
    public static function setSegment(int handle, int x, int y, int width, int height, int value) -> void { fb_set_segment(handle, x, y, width, height, value); }
    public static function fill(int handle, int value) -> void { fb_fill(handle, value); }

    public static function bytes(int handle) -> string
    {
        var result;
        let result = fb_bytes(handle);
        return result;
    }

    public static function region(int handle, int x, int y, int width, int height, int scan) -> string
    {
        var result;
        let result = fb_region(handle, x, y, width, height, scan);
        return result;
    }

    public static function layer(int handle, int layer) -> string
    {
        var result;
        let result = fb_layer(handle, layer);
        return result;
    }

    public static function pointer(int handle) -> int { return fb_pointer(handle); }

    public static function toRgba8(int handle) -> string
    {
        var result;
        let result = fb_to_rgba8(handle);
        return result;
    }

    public static function blitRgba8(int handle, string rgba8, int srcWidth, int srcHeight, int offsetX, int offsetY) -> void
    {
        fb_blit_rgba8(handle, rgba8, srcWidth, srcHeight, offsetX, offsetY);
    }

    public static function transcode(int handle, int pixelFormat, int bitDepth, int bitOrder, int endianness, int pageAxis, int scan, array palette, int x, int y, int width, int height) -> string
    {
        var result;
        let result = fb_transcode(handle, pixelFormat, bitDepth, bitOrder, endianness, pageAxis, scan, palette, x, y, width, height);
        return result;
    }

    public static function granularity(int handle) -> array
    {
        var result;
        let result = fb_granularity(handle);
        return result;
    }

    public static function beginEpoch(int handle) -> void { fb_begin_epoch(handle); }

    public static function damage(int handle) -> array
    {
        var result;
        let result = fb_damage(handle);
        return result;
    }

    public static function setPage(int handle, int page) -> void { fb_set_page(handle, page); }
    public static function page(int handle) -> int { return fb_page(handle); }
    public static function pages(int handle) -> int { return fb_pages(handle); }
    public static function present(int handle) -> void { fb_present(handle); }
    public static function frames(int handle) -> int { return fb_frames(handle); }
}
