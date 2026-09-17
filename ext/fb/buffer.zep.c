
#ifdef HAVE_CONFIG_H
#include "../ext_config.h"
#endif

#include <php.h>
#include "../php_ext.h"
#include "../ext.h"

#include <Zend/zend_operators.h>
#include <Zend/zend_exceptions.h>
#include <Zend/zend_interfaces.h>

#include "kernel/main.h"
#include "src/fb-api.h"
#include "kernel/operators.h"
#include "kernel/memory.h"
#include "kernel/object.h"
#include "kernel/string.h"


/**
 * Five framebuffer kinds in C, addressed by int handles. Every method is one
 * C call; jovian/fb owns the handle's lifetime and the Surface contract.
 */
ZEPHIR_INIT_CLASS(Fb_Buffer)
{
	ZEPHIR_REGISTER_CLASS(Fb, Buffer, fb, buffer, fb_buffer_method_entry, 0);

	return SUCCESS;
}

PHP_METHOD(Fb_Buffer, create)
{
	zephir_method_globals *ZEPHIR_METHOD_GLOBALS_PTR = NULL;
	zval palette;
	zval *kind_param = NULL, *pixelFormat_param = NULL, *bitDepth_param = NULL, *bitOrder_param = NULL, *endianness_param = NULL, *pageAxis_param = NULL, *scan_param = NULL, *palette_param = NULL, *width_param = NULL, *height_param = NULL, *extra_param = NULL, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9;
	zend_long kind, pixelFormat, bitDepth, bitOrder, endianness, pageAxis, scan, width, height, extra;

	ZVAL_UNDEF(&_0);
	ZVAL_UNDEF(&_1);
	ZVAL_UNDEF(&_2);
	ZVAL_UNDEF(&_3);
	ZVAL_UNDEF(&_4);
	ZVAL_UNDEF(&_5);
	ZVAL_UNDEF(&_6);
	ZVAL_UNDEF(&_7);
	ZVAL_UNDEF(&_8);
	ZVAL_UNDEF(&_9);
	ZVAL_UNDEF(&palette);
	ZEND_PARSE_PARAMETERS_START(11, 11)
		Z_PARAM_LONG(kind)
		Z_PARAM_LONG(pixelFormat)
		Z_PARAM_LONG(bitDepth)
		Z_PARAM_LONG(bitOrder)
		Z_PARAM_LONG(endianness)
		Z_PARAM_LONG(pageAxis)
		Z_PARAM_LONG(scan)
		Z_PARAM_ARRAY(palette)
		Z_PARAM_LONG(width)
		Z_PARAM_LONG(height)
		Z_PARAM_LONG(extra)
	ZEND_PARSE_PARAMETERS_END();
	ZEPHIR_METHOD_GLOBALS_PTR = pecalloc(1, sizeof(zephir_method_globals), 0);
	zephir_memory_grow_stack(ZEPHIR_METHOD_GLOBALS_PTR, __func__);
	zephir_fetch_params(1, 11, 0, &kind_param, &pixelFormat_param, &bitDepth_param, &bitOrder_param, &endianness_param, &pageAxis_param, &scan_param, &palette_param, &width_param, &height_param, &extra_param);
	zephir_get_arrval(&palette, palette_param);
	ZVAL_LONG(&_0, kind);
	ZVAL_LONG(&_1, pixelFormat);
	ZVAL_LONG(&_2, bitDepth);
	ZVAL_LONG(&_3, bitOrder);
	ZVAL_LONG(&_4, endianness);
	ZVAL_LONG(&_5, pageAxis);
	ZVAL_LONG(&_6, scan);
	ZVAL_LONG(&_7, width);
	ZVAL_LONG(&_8, height);
	ZVAL_LONG(&_9, extra);
	RETURN_MM_LONG(fb_api_create(&_0, &_1, &_2, &_3, &_4, &_5, &_6, &palette, &_7, &_8, &_9));
}

PHP_METHOD(Fb_Buffer, free)
{
	zval *handle_param = NULL, _0;
	zend_long handle;

	ZVAL_UNDEF(&_0);
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(handle)
	ZEND_PARSE_PARAMETERS_END();
	zephir_fetch_params_without_memory_grow(1, 0, &handle_param);
	ZVAL_LONG(&_0, handle);
	fb_api_free(&_0);
}

PHP_METHOD(Fb_Buffer, width)
{
	zval *handle_param = NULL, _0;
	zend_long handle;

	ZVAL_UNDEF(&_0);
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(handle)
	ZEND_PARSE_PARAMETERS_END();
	zephir_fetch_params_without_memory_grow(1, 0, &handle_param);
	ZVAL_LONG(&_0, handle);
	RETURN_LONG(fb_api_width(&_0));
}

PHP_METHOD(Fb_Buffer, height)
{
	zval *handle_param = NULL, _0;
	zend_long handle;

	ZVAL_UNDEF(&_0);
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(handle)
	ZEND_PARSE_PARAMETERS_END();
	zephir_fetch_params_without_memory_grow(1, 0, &handle_param);
	ZVAL_LONG(&_0, handle);
	RETURN_LONG(fb_api_height(&_0));
}

PHP_METHOD(Fb_Buffer, size)
{
	zval *handle_param = NULL, _0;
	zend_long handle;

	ZVAL_UNDEF(&_0);
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(handle)
	ZEND_PARSE_PARAMETERS_END();
	zephir_fetch_params_without_memory_grow(1, 0, &handle_param);
	ZVAL_LONG(&_0, handle);
	RETURN_LONG(fb_api_size(&_0));
}

PHP_METHOD(Fb_Buffer, get)
{
	zval *handle_param = NULL, *x_param = NULL, *y_param = NULL, _0, _1, _2;
	zend_long handle, x, y;

	ZVAL_UNDEF(&_0);
	ZVAL_UNDEF(&_1);
	ZVAL_UNDEF(&_2);
	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_LONG(handle)
		Z_PARAM_LONG(x)
		Z_PARAM_LONG(y)
	ZEND_PARSE_PARAMETERS_END();
	zephir_fetch_params_without_memory_grow(3, 0, &handle_param, &x_param, &y_param);
	ZVAL_LONG(&_0, handle);
	ZVAL_LONG(&_1, x);
	ZVAL_LONG(&_2, y);
	RETURN_LONG(fb_api_get(&_0, &_1, &_2));
}

PHP_METHOD(Fb_Buffer, set)
{
	zval *handle_param = NULL, *x_param = NULL, *y_param = NULL, *value_param = NULL, _0, _1, _2, _3;
	zend_long handle, x, y, value;

	ZVAL_UNDEF(&_0);
	ZVAL_UNDEF(&_1);
	ZVAL_UNDEF(&_2);
	ZVAL_UNDEF(&_3);
	ZEND_PARSE_PARAMETERS_START(4, 4)
		Z_PARAM_LONG(handle)
		Z_PARAM_LONG(x)
		Z_PARAM_LONG(y)
		Z_PARAM_LONG(value)
	ZEND_PARSE_PARAMETERS_END();
	zephir_fetch_params_without_memory_grow(4, 0, &handle_param, &x_param, &y_param, &value_param);
	ZVAL_LONG(&_0, handle);
	ZVAL_LONG(&_1, x);
	ZVAL_LONG(&_2, y);
	ZVAL_LONG(&_3, value);
	fb_api_set(&_0, &_1, &_2, &_3);
}

PHP_METHOD(Fb_Buffer, setPixels)
{
	zephir_method_globals *ZEPHIR_METHOD_GLOBALS_PTR = NULL;
	zval pixels;
	zval *handle_param = NULL, *pixels_param = NULL, _0;
	zend_long handle;

	ZVAL_UNDEF(&_0);
	ZVAL_UNDEF(&pixels);
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_LONG(handle)
		Z_PARAM_ARRAY(pixels)
	ZEND_PARSE_PARAMETERS_END();
	ZEPHIR_METHOD_GLOBALS_PTR = pecalloc(1, sizeof(zephir_method_globals), 0);
	zephir_memory_grow_stack(ZEPHIR_METHOD_GLOBALS_PTR, __func__);
	zephir_fetch_params(1, 2, 0, &handle_param, &pixels_param);
	zephir_get_arrval(&pixels, pixels_param);
	ZVAL_LONG(&_0, handle);
	fb_api_set_pixels(&_0, &pixels);
	ZEPHIR_MM_RESTORE();
}

PHP_METHOD(Fb_Buffer, setSegment)
{
	zval *handle_param = NULL, *x_param = NULL, *y_param = NULL, *width_param = NULL, *height_param = NULL, *value_param = NULL, _0, _1, _2, _3, _4, _5;
	zend_long handle, x, y, width, height, value;

	ZVAL_UNDEF(&_0);
	ZVAL_UNDEF(&_1);
	ZVAL_UNDEF(&_2);
	ZVAL_UNDEF(&_3);
	ZVAL_UNDEF(&_4);
	ZVAL_UNDEF(&_5);
	ZEND_PARSE_PARAMETERS_START(6, 6)
		Z_PARAM_LONG(handle)
		Z_PARAM_LONG(x)
		Z_PARAM_LONG(y)
		Z_PARAM_LONG(width)
		Z_PARAM_LONG(height)
		Z_PARAM_LONG(value)
	ZEND_PARSE_PARAMETERS_END();
	zephir_fetch_params_without_memory_grow(6, 0, &handle_param, &x_param, &y_param, &width_param, &height_param, &value_param);
	ZVAL_LONG(&_0, handle);
	ZVAL_LONG(&_1, x);
	ZVAL_LONG(&_2, y);
	ZVAL_LONG(&_3, width);
	ZVAL_LONG(&_4, height);
	ZVAL_LONG(&_5, value);
	fb_api_set_segment(&_0, &_1, &_2, &_3, &_4, &_5);
}

PHP_METHOD(Fb_Buffer, fill)
{
	zval *handle_param = NULL, *value_param = NULL, _0, _1;
	zend_long handle, value;

	ZVAL_UNDEF(&_0);
	ZVAL_UNDEF(&_1);
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_LONG(handle)
		Z_PARAM_LONG(value)
	ZEND_PARSE_PARAMETERS_END();
	zephir_fetch_params_without_memory_grow(2, 0, &handle_param, &value_param);
	ZVAL_LONG(&_0, handle);
	ZVAL_LONG(&_1, value);
	fb_api_fill(&_0, &_1);
}

PHP_METHOD(Fb_Buffer, bytes)
{
	zephir_method_globals *ZEPHIR_METHOD_GLOBALS_PTR = NULL;
	zval *handle_param = NULL, result, _0;
	zend_long handle;

	ZVAL_UNDEF(&result);
	ZVAL_UNDEF(&_0);
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(handle)
	ZEND_PARSE_PARAMETERS_END();
	ZEPHIR_METHOD_GLOBALS_PTR = pecalloc(1, sizeof(zephir_method_globals), 0);
	zephir_memory_grow_stack(ZEPHIR_METHOD_GLOBALS_PTR, __func__);
	zephir_fetch_params(1, 1, 0, &handle_param);
	ZVAL_LONG(&_0, handle);
	ZEPHIR_INIT_VAR(&result);
	fb_api_bytes(&result, &_0);
	RETURN_CCTOR(&result);
}

PHP_METHOD(Fb_Buffer, region)
{
	zephir_method_globals *ZEPHIR_METHOD_GLOBALS_PTR = NULL;
	zval *handle_param = NULL, *x_param = NULL, *y_param = NULL, *width_param = NULL, *height_param = NULL, *scan_param = NULL, result, _0, _1, _2, _3, _4, _5;
	zend_long handle, x, y, width, height, scan;

	ZVAL_UNDEF(&result);
	ZVAL_UNDEF(&_0);
	ZVAL_UNDEF(&_1);
	ZVAL_UNDEF(&_2);
	ZVAL_UNDEF(&_3);
	ZVAL_UNDEF(&_4);
	ZVAL_UNDEF(&_5);
	ZEND_PARSE_PARAMETERS_START(6, 6)
		Z_PARAM_LONG(handle)
		Z_PARAM_LONG(x)
		Z_PARAM_LONG(y)
		Z_PARAM_LONG(width)
		Z_PARAM_LONG(height)
		Z_PARAM_LONG(scan)
	ZEND_PARSE_PARAMETERS_END();
	ZEPHIR_METHOD_GLOBALS_PTR = pecalloc(1, sizeof(zephir_method_globals), 0);
	zephir_memory_grow_stack(ZEPHIR_METHOD_GLOBALS_PTR, __func__);
	zephir_fetch_params(1, 6, 0, &handle_param, &x_param, &y_param, &width_param, &height_param, &scan_param);
	ZVAL_LONG(&_0, handle);
	ZVAL_LONG(&_1, x);
	ZVAL_LONG(&_2, y);
	ZVAL_LONG(&_3, width);
	ZVAL_LONG(&_4, height);
	ZVAL_LONG(&_5, scan);
	ZEPHIR_INIT_VAR(&result);
	fb_api_region(&result, &_0, &_1, &_2, &_3, &_4, &_5);
	RETURN_CCTOR(&result);
}

PHP_METHOD(Fb_Buffer, layer)
{
	zephir_method_globals *ZEPHIR_METHOD_GLOBALS_PTR = NULL;
	zval *handle_param = NULL, *layer_param = NULL, result, _0, _1;
	zend_long handle, layer;

	ZVAL_UNDEF(&result);
	ZVAL_UNDEF(&_0);
	ZVAL_UNDEF(&_1);
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_LONG(handle)
		Z_PARAM_LONG(layer)
	ZEND_PARSE_PARAMETERS_END();
	ZEPHIR_METHOD_GLOBALS_PTR = pecalloc(1, sizeof(zephir_method_globals), 0);
	zephir_memory_grow_stack(ZEPHIR_METHOD_GLOBALS_PTR, __func__);
	zephir_fetch_params(1, 2, 0, &handle_param, &layer_param);
	ZVAL_LONG(&_0, handle);
	ZVAL_LONG(&_1, layer);
	ZEPHIR_INIT_VAR(&result);
	fb_api_layer(&result, &_0, &_1);
	RETURN_CCTOR(&result);
}

PHP_METHOD(Fb_Buffer, pointer)
{
	zval *handle_param = NULL, _0;
	zend_long handle;

	ZVAL_UNDEF(&_0);
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(handle)
	ZEND_PARSE_PARAMETERS_END();
	zephir_fetch_params_without_memory_grow(1, 0, &handle_param);
	ZVAL_LONG(&_0, handle);
	RETURN_LONG(fb_api_pointer(&_0));
}

PHP_METHOD(Fb_Buffer, toRgba8)
{
	zephir_method_globals *ZEPHIR_METHOD_GLOBALS_PTR = NULL;
	zval *handle_param = NULL, result, _0;
	zend_long handle;

	ZVAL_UNDEF(&result);
	ZVAL_UNDEF(&_0);
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(handle)
	ZEND_PARSE_PARAMETERS_END();
	ZEPHIR_METHOD_GLOBALS_PTR = pecalloc(1, sizeof(zephir_method_globals), 0);
	zephir_memory_grow_stack(ZEPHIR_METHOD_GLOBALS_PTR, __func__);
	zephir_fetch_params(1, 1, 0, &handle_param);
	ZVAL_LONG(&_0, handle);
	ZEPHIR_INIT_VAR(&result);
	fb_api_to_rgba8(&result, &_0);
	RETURN_CCTOR(&result);
}

PHP_METHOD(Fb_Buffer, blitRgba8)
{
	zephir_method_globals *ZEPHIR_METHOD_GLOBALS_PTR = NULL;
	zval rgba8;
	zval *handle_param = NULL, *rgba8_param = NULL, *srcWidth_param = NULL, *srcHeight_param = NULL, *offsetX_param = NULL, *offsetY_param = NULL, _0, _1, _2, _3, _4;
	zend_long handle, srcWidth, srcHeight, offsetX, offsetY;

	ZVAL_UNDEF(&_0);
	ZVAL_UNDEF(&_1);
	ZVAL_UNDEF(&_2);
	ZVAL_UNDEF(&_3);
	ZVAL_UNDEF(&_4);
	ZVAL_UNDEF(&rgba8);
	ZEND_PARSE_PARAMETERS_START(6, 6)
		Z_PARAM_LONG(handle)
		Z_PARAM_STR(rgba8)
		Z_PARAM_LONG(srcWidth)
		Z_PARAM_LONG(srcHeight)
		Z_PARAM_LONG(offsetX)
		Z_PARAM_LONG(offsetY)
	ZEND_PARSE_PARAMETERS_END();
	ZEPHIR_METHOD_GLOBALS_PTR = pecalloc(1, sizeof(zephir_method_globals), 0);
	zephir_memory_grow_stack(ZEPHIR_METHOD_GLOBALS_PTR, __func__);
	zephir_fetch_params(1, 6, 0, &handle_param, &rgba8_param, &srcWidth_param, &srcHeight_param, &offsetX_param, &offsetY_param);
	zephir_get_strval(&rgba8, rgba8_param);
	ZVAL_LONG(&_0, handle);
	ZVAL_LONG(&_1, srcWidth);
	ZVAL_LONG(&_2, srcHeight);
	ZVAL_LONG(&_3, offsetX);
	ZVAL_LONG(&_4, offsetY);
	fb_api_blit_rgba8(&_0, &rgba8, &_1, &_2, &_3, &_4);
	ZEPHIR_MM_RESTORE();
}

PHP_METHOD(Fb_Buffer, transcode)
{
	zephir_method_globals *ZEPHIR_METHOD_GLOBALS_PTR = NULL;
	zval palette;
	zval *handle_param = NULL, *pixelFormat_param = NULL, *bitDepth_param = NULL, *bitOrder_param = NULL, *endianness_param = NULL, *pageAxis_param = NULL, *scan_param = NULL, *palette_param = NULL, *x_param = NULL, *y_param = NULL, *width_param = NULL, *height_param = NULL, result, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10;
	zend_long handle, pixelFormat, bitDepth, bitOrder, endianness, pageAxis, scan, x, y, width, height;

	ZVAL_UNDEF(&result);
	ZVAL_UNDEF(&_0);
	ZVAL_UNDEF(&_1);
	ZVAL_UNDEF(&_2);
	ZVAL_UNDEF(&_3);
	ZVAL_UNDEF(&_4);
	ZVAL_UNDEF(&_5);
	ZVAL_UNDEF(&_6);
	ZVAL_UNDEF(&_7);
	ZVAL_UNDEF(&_8);
	ZVAL_UNDEF(&_9);
	ZVAL_UNDEF(&_10);
	ZVAL_UNDEF(&palette);
	ZEND_PARSE_PARAMETERS_START(12, 12)
		Z_PARAM_LONG(handle)
		Z_PARAM_LONG(pixelFormat)
		Z_PARAM_LONG(bitDepth)
		Z_PARAM_LONG(bitOrder)
		Z_PARAM_LONG(endianness)
		Z_PARAM_LONG(pageAxis)
		Z_PARAM_LONG(scan)
		Z_PARAM_ARRAY(palette)
		Z_PARAM_LONG(x)
		Z_PARAM_LONG(y)
		Z_PARAM_LONG(width)
		Z_PARAM_LONG(height)
	ZEND_PARSE_PARAMETERS_END();
	ZEPHIR_METHOD_GLOBALS_PTR = pecalloc(1, sizeof(zephir_method_globals), 0);
	zephir_memory_grow_stack(ZEPHIR_METHOD_GLOBALS_PTR, __func__);
	zephir_fetch_params(1, 12, 0, &handle_param, &pixelFormat_param, &bitDepth_param, &bitOrder_param, &endianness_param, &pageAxis_param, &scan_param, &palette_param, &x_param, &y_param, &width_param, &height_param);
	zephir_get_arrval(&palette, palette_param);
	ZVAL_LONG(&_0, handle);
	ZVAL_LONG(&_1, pixelFormat);
	ZVAL_LONG(&_2, bitDepth);
	ZVAL_LONG(&_3, bitOrder);
	ZVAL_LONG(&_4, endianness);
	ZVAL_LONG(&_5, pageAxis);
	ZVAL_LONG(&_6, scan);
	ZVAL_LONG(&_7, x);
	ZVAL_LONG(&_8, y);
	ZVAL_LONG(&_9, width);
	ZVAL_LONG(&_10, height);
	ZEPHIR_INIT_VAR(&result);
	fb_api_transcode(&result, &_0, &_1, &_2, &_3, &_4, &_5, &_6, &palette, &_7, &_8, &_9, &_10);
	RETURN_CCTOR(&result);
}

PHP_METHOD(Fb_Buffer, granularity)
{
	zephir_method_globals *ZEPHIR_METHOD_GLOBALS_PTR = NULL;
	zval *handle_param = NULL, result, _0;
	zend_long handle;

	ZVAL_UNDEF(&result);
	ZVAL_UNDEF(&_0);
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(handle)
	ZEND_PARSE_PARAMETERS_END();
	ZEPHIR_METHOD_GLOBALS_PTR = pecalloc(1, sizeof(zephir_method_globals), 0);
	zephir_memory_grow_stack(ZEPHIR_METHOD_GLOBALS_PTR, __func__);
	zephir_fetch_params(1, 1, 0, &handle_param);
	ZVAL_LONG(&_0, handle);
	ZEPHIR_INIT_VAR(&result);
	fb_api_granularity(&result, &_0);
	RETURN_CCTOR(&result);
}

PHP_METHOD(Fb_Buffer, beginEpoch)
{
	zval *handle_param = NULL, _0;
	zend_long handle;

	ZVAL_UNDEF(&_0);
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(handle)
	ZEND_PARSE_PARAMETERS_END();
	zephir_fetch_params_without_memory_grow(1, 0, &handle_param);
	ZVAL_LONG(&_0, handle);
	fb_api_begin_epoch(&_0);
}

PHP_METHOD(Fb_Buffer, damage)
{
	zephir_method_globals *ZEPHIR_METHOD_GLOBALS_PTR = NULL;
	zval *handle_param = NULL, result, _0;
	zend_long handle;

	ZVAL_UNDEF(&result);
	ZVAL_UNDEF(&_0);
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(handle)
	ZEND_PARSE_PARAMETERS_END();
	ZEPHIR_METHOD_GLOBALS_PTR = pecalloc(1, sizeof(zephir_method_globals), 0);
	zephir_memory_grow_stack(ZEPHIR_METHOD_GLOBALS_PTR, __func__);
	zephir_fetch_params(1, 1, 0, &handle_param);
	ZVAL_LONG(&_0, handle);
	ZEPHIR_INIT_VAR(&result);
	fb_api_damage(&result, &_0);
	RETURN_CCTOR(&result);
}

PHP_METHOD(Fb_Buffer, setPage)
{
	zval *handle_param = NULL, *page_param = NULL, _0, _1;
	zend_long handle, page;

	ZVAL_UNDEF(&_0);
	ZVAL_UNDEF(&_1);
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_LONG(handle)
		Z_PARAM_LONG(page)
	ZEND_PARSE_PARAMETERS_END();
	zephir_fetch_params_without_memory_grow(2, 0, &handle_param, &page_param);
	ZVAL_LONG(&_0, handle);
	ZVAL_LONG(&_1, page);
	fb_api_set_page(&_0, &_1);
}

PHP_METHOD(Fb_Buffer, page)
{
	zval *handle_param = NULL, _0;
	zend_long handle;

	ZVAL_UNDEF(&_0);
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(handle)
	ZEND_PARSE_PARAMETERS_END();
	zephir_fetch_params_without_memory_grow(1, 0, &handle_param);
	ZVAL_LONG(&_0, handle);
	RETURN_LONG(fb_api_page(&_0));
}

PHP_METHOD(Fb_Buffer, pages)
{
	zval *handle_param = NULL, _0;
	zend_long handle;

	ZVAL_UNDEF(&_0);
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(handle)
	ZEND_PARSE_PARAMETERS_END();
	zephir_fetch_params_without_memory_grow(1, 0, &handle_param);
	ZVAL_LONG(&_0, handle);
	RETURN_LONG(fb_api_pages(&_0));
}

PHP_METHOD(Fb_Buffer, present)
{
	zval *handle_param = NULL, _0;
	zend_long handle;

	ZVAL_UNDEF(&_0);
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(handle)
	ZEND_PARSE_PARAMETERS_END();
	zephir_fetch_params_without_memory_grow(1, 0, &handle_param);
	ZVAL_LONG(&_0, handle);
	fb_api_present(&_0);
}

PHP_METHOD(Fb_Buffer, frames)
{
	zval *handle_param = NULL, _0;
	zend_long handle;

	ZVAL_UNDEF(&_0);
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(handle)
	ZEND_PARSE_PARAMETERS_END();
	zephir_fetch_params_without_memory_grow(1, 0, &handle_param);
	ZVAL_LONG(&_0, handle);
	RETURN_LONG(fb_api_frames(&_0));
}

