/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 541488ed5223765d05766f13693fa9b4ec37074d */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_FbBuffer___construct, 0, 0, 3)
	ZEND_ARG_OBJ_INFO(0, format, FbFormat, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_FbBuffer_over, 0, 5, FbBuffer, 0)
	ZEND_ARG_OBJ_INFO(0, format, FbFormat, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, address, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, owner, IS_OBJECT, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_FbBuffer_format, 0, 0, FbFormat, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_width, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FbBuffer_height arginfo_class_FbBuffer_width

#define arginfo_class_FbBuffer_size arginfo_class_FbBuffer_width

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_get, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_set, 0, 3, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_setPixels, 0, 1, IS_ARRAY, 1)
	ZEND_ARG_TYPE_INFO(0, pixels, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_setCoordinates, 0, 2, IS_ARRAY, 1)
	ZEND_ARG_TYPE_INFO(0, coordinates, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_rect, 0, 5, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_fill, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_bytes, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_layer, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, layer, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_region, 0, 4, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
	ZEND_ARG_OBJ_INFO_WITH_DEFAULT_VALUE(0, format, FbFormat, 1, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_FbBuffer_rgba8 arginfo_class_FbBuffer_bytes

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_blitRgba8, 0, 5, IS_ARRAY, 1)
	ZEND_ARG_TYPE_INFO(0, rgba8, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, clipX, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, clipY, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, clipWidth, IS_LONG, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, clipHeight, IS_LONG, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_copy, 0, 5, IS_VOID, 0)
	ZEND_ARG_OBJ_INFO(0, source, FbBuffer, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_plane, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_paintSpans, 0, 2, IS_ARRAY, 1)
	ZEND_ARG_TYPE_INFO(0, spans, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, rgba, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_paintMasks, 0, 6, IS_ARRAY, 1)
	ZEND_ARG_TYPE_INFO(0, masks, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, rgba, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, clipX, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, clipY, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, clipWidth, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, clipHeight, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_filter, 0, 6, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, steps, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, maps, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, row, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, premultiplied, _IS_BOOL, 0, "false")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_paintGradient, 0, 5, IS_ARRAY, 1)
	ZEND_ARG_TYPE_INFO(0, spans, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, ramp, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, radial, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, spread, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, inverse, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, row, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_paintRgba8, 0, 8, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, rgba8, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, inverse, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, targetWidth, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, targetHeight, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, opacity, IS_LONG, 0, "255")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, smooth, _IS_BOOL, 0, "false")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, row, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, tint, IS_LONG, 0, "0xffffffff")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, window, IS_ARRAY, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, coverage, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_FbBuffer_kawaseDown arginfo_class_FbBuffer_copy

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_kawaseUp, 0, 1, IS_VOID, 0)
	ZEND_ARG_OBJ_INFO(0, source, FbBuffer, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_paintGlass, 0, 12, IS_ARRAY, 1)
	ZEND_ARG_TYPE_INFO(0, box, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, radii, IS_ARRAY, 0)
	ZEND_ARG_OBJ_INFO(0, blur, FbBuffer, 0)
	ZEND_ARG_TYPE_INFO(0, scale, IS_LONG, 0)
	ZEND_ARG_OBJ_INFO(0, sharp, FbBuffer, 1)
	ZEND_ARG_TYPE_INFO(0, originX, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, originY, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, look, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, clipX, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, clipY, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, clipWidth, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, clipHeight, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FbBuffer_pointer arginfo_class_FbBuffer_width

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbBuffer_granularity, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_METHOD(FbBuffer, __construct);
ZEND_METHOD(FbBuffer, over);
ZEND_METHOD(FbBuffer, format);
ZEND_METHOD(FbBuffer, width);
ZEND_METHOD(FbBuffer, height);
ZEND_METHOD(FbBuffer, size);
ZEND_METHOD(FbBuffer, get);
ZEND_METHOD(FbBuffer, set);
ZEND_METHOD(FbBuffer, setPixels);
ZEND_METHOD(FbBuffer, setCoordinates);
ZEND_METHOD(FbBuffer, rect);
ZEND_METHOD(FbBuffer, fill);
ZEND_METHOD(FbBuffer, bytes);
ZEND_METHOD(FbBuffer, layer);
ZEND_METHOD(FbBuffer, region);
ZEND_METHOD(FbBuffer, rgba8);
ZEND_METHOD(FbBuffer, blitRgba8);
ZEND_METHOD(FbBuffer, copy);
ZEND_METHOD(FbBuffer, plane);
ZEND_METHOD(FbBuffer, paintSpans);
ZEND_METHOD(FbBuffer, paintMasks);
ZEND_METHOD(FbBuffer, filter);
ZEND_METHOD(FbBuffer, paintGradient);
ZEND_METHOD(FbBuffer, paintRgba8);
ZEND_METHOD(FbBuffer, kawaseDown);
ZEND_METHOD(FbBuffer, kawaseUp);
ZEND_METHOD(FbBuffer, paintGlass);
ZEND_METHOD(FbBuffer, pointer);
ZEND_METHOD(FbBuffer, granularity);

static const zend_function_entry class_FbBuffer_methods[] = {
	ZEND_ME(FbBuffer, __construct, arginfo_class_FbBuffer___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, over, arginfo_class_FbBuffer_over, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(FbBuffer, format, arginfo_class_FbBuffer_format, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, width, arginfo_class_FbBuffer_width, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, height, arginfo_class_FbBuffer_height, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, size, arginfo_class_FbBuffer_size, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, get, arginfo_class_FbBuffer_get, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, set, arginfo_class_FbBuffer_set, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, setPixels, arginfo_class_FbBuffer_setPixels, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, setCoordinates, arginfo_class_FbBuffer_setCoordinates, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, rect, arginfo_class_FbBuffer_rect, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, fill, arginfo_class_FbBuffer_fill, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, bytes, arginfo_class_FbBuffer_bytes, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, layer, arginfo_class_FbBuffer_layer, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, region, arginfo_class_FbBuffer_region, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, rgba8, arginfo_class_FbBuffer_rgba8, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, blitRgba8, arginfo_class_FbBuffer_blitRgba8, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, copy, arginfo_class_FbBuffer_copy, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, plane, arginfo_class_FbBuffer_plane, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, paintSpans, arginfo_class_FbBuffer_paintSpans, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, paintMasks, arginfo_class_FbBuffer_paintMasks, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, filter, arginfo_class_FbBuffer_filter, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, paintGradient, arginfo_class_FbBuffer_paintGradient, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, paintRgba8, arginfo_class_FbBuffer_paintRgba8, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, kawaseDown, arginfo_class_FbBuffer_kawaseDown, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, kawaseUp, arginfo_class_FbBuffer_kawaseUp, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, paintGlass, arginfo_class_FbBuffer_paintGlass, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, pointer, arginfo_class_FbBuffer_pointer, ZEND_ACC_PUBLIC)
	ZEND_ME(FbBuffer, granularity, arginfo_class_FbBuffer_granularity, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_FbBuffer(void)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "FbBuffer", class_FbBuffer_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, ZEND_ACC_FINAL|ZEND_ACC_NOT_SERIALIZABLE);

	return class_entry;
}
