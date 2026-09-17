
extern zend_class_entry *fb_buffer_ce;

ZEPHIR_INIT_CLASS(Fb_Buffer);

PHP_METHOD(Fb_Buffer, create);
PHP_METHOD(Fb_Buffer, free);
PHP_METHOD(Fb_Buffer, width);
PHP_METHOD(Fb_Buffer, height);
PHP_METHOD(Fb_Buffer, size);
PHP_METHOD(Fb_Buffer, get);
PHP_METHOD(Fb_Buffer, set);
PHP_METHOD(Fb_Buffer, setPixels);
PHP_METHOD(Fb_Buffer, setSegment);
PHP_METHOD(Fb_Buffer, fill);
PHP_METHOD(Fb_Buffer, bytes);
PHP_METHOD(Fb_Buffer, region);
PHP_METHOD(Fb_Buffer, layer);
PHP_METHOD(Fb_Buffer, pointer);
PHP_METHOD(Fb_Buffer, toRgba8);
PHP_METHOD(Fb_Buffer, blitRgba8);
PHP_METHOD(Fb_Buffer, transcode);
PHP_METHOD(Fb_Buffer, granularity);
PHP_METHOD(Fb_Buffer, beginEpoch);
PHP_METHOD(Fb_Buffer, damage);
PHP_METHOD(Fb_Buffer, setPage);
PHP_METHOD(Fb_Buffer, page);
PHP_METHOD(Fb_Buffer, pages);
PHP_METHOD(Fb_Buffer, present);
PHP_METHOD(Fb_Buffer, frames);

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_create, 0, 11, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, kind, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, pixelFormat, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, bitDepth, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, bitOrder, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, endianness, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, pageAxis, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, scan, IS_LONG, 0)
	ZEND_ARG_ARRAY_INFO(0, palette, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, extra, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_free, 0, 1, IS_VOID, 0)

	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_width, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_height, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_size, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_get, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_set, 0, 4, IS_VOID, 0)

	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_setpixels, 0, 2, IS_VOID, 0)

	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
	ZEND_ARG_ARRAY_INFO(0, pixels, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_setsegment, 0, 6, IS_VOID, 0)

	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_fill, 0, 2, IS_VOID, 0)

	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_bytes, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_region, 0, 6, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, scan, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_layer, 0, 2, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, layer, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_pointer, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_torgba8, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_blitrgba8, 0, 6, IS_VOID, 0)

	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, rgba8, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, srcWidth, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, srcHeight, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, offsetX, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, offsetY, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_transcode, 0, 12, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, pixelFormat, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, bitDepth, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, bitOrder, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, endianness, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, pageAxis, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, scan, IS_LONG, 0)
	ZEND_ARG_ARRAY_INFO(0, palette, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_granularity, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_beginepoch, 0, 1, IS_VOID, 0)

	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_damage, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_setpage, 0, 2, IS_VOID, 0)

	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, page, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_page, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_pages, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_present, 0, 1, IS_VOID, 0)

	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fb_buffer_frames, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, handle, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEPHIR_INIT_FUNCS(fb_buffer_method_entry) {
	PHP_ME(Fb_Buffer, create, arginfo_fb_buffer_create, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, free, arginfo_fb_buffer_free, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, width, arginfo_fb_buffer_width, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, height, arginfo_fb_buffer_height, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, size, arginfo_fb_buffer_size, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, get, arginfo_fb_buffer_get, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, set, arginfo_fb_buffer_set, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, setPixels, arginfo_fb_buffer_setpixels, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, setSegment, arginfo_fb_buffer_setsegment, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, fill, arginfo_fb_buffer_fill, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, bytes, arginfo_fb_buffer_bytes, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, region, arginfo_fb_buffer_region, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, layer, arginfo_fb_buffer_layer, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, pointer, arginfo_fb_buffer_pointer, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, toRgba8, arginfo_fb_buffer_torgba8, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, blitRgba8, arginfo_fb_buffer_blitrgba8, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, transcode, arginfo_fb_buffer_transcode, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, granularity, arginfo_fb_buffer_granularity, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, beginEpoch, arginfo_fb_buffer_beginepoch, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, damage, arginfo_fb_buffer_damage, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, setPage, arginfo_fb_buffer_setpage, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, page, arginfo_fb_buffer_page, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, pages, arginfo_fb_buffer_pages, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, present, arginfo_fb_buffer_present, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Fb_Buffer, frames, arginfo_fb_buffer_frames, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_FE_END
};
