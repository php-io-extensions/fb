/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 3734eca201b49cc8eb54019b13ca12013e592e4d */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_FbFormat___construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, layout, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, bitOrder, IS_LONG, 0, "FB_BIT_ORDER_MSB")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, byteOrder, IS_LONG, 0, "FB_BYTE_ORDER_MSB")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, channelOrder, IS_LONG, 0, "FB_CHANNELS_RGB")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, scan, IS_LONG, 0, "FB_SCAN_TOP_DOWN")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, palette, IS_ARRAY, 0, "[]")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbFormat_layout, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FbFormat_bitOrder arginfo_class_FbFormat_layout

#define arginfo_class_FbFormat_byteOrder arginfo_class_FbFormat_layout

#define arginfo_class_FbFormat_channelOrder arginfo_class_FbFormat_layout

#define arginfo_class_FbFormat_scan arginfo_class_FbFormat_layout

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbFormat_size, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbFormat_mapRgba8, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, red, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, green, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, blue, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, alpha, IS_LONG, 0, "255")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FbFormat_unmapRgba8, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, word, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_METHOD(FbFormat, __construct);
ZEND_METHOD(FbFormat, layout);
ZEND_METHOD(FbFormat, bitOrder);
ZEND_METHOD(FbFormat, byteOrder);
ZEND_METHOD(FbFormat, channelOrder);
ZEND_METHOD(FbFormat, scan);
ZEND_METHOD(FbFormat, size);
ZEND_METHOD(FbFormat, mapRgba8);
ZEND_METHOD(FbFormat, unmapRgba8);

static const zend_function_entry class_FbFormat_methods[] = {
	ZEND_ME(FbFormat, __construct, arginfo_class_FbFormat___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(FbFormat, layout, arginfo_class_FbFormat_layout, ZEND_ACC_PUBLIC)
	ZEND_ME(FbFormat, bitOrder, arginfo_class_FbFormat_bitOrder, ZEND_ACC_PUBLIC)
	ZEND_ME(FbFormat, byteOrder, arginfo_class_FbFormat_byteOrder, ZEND_ACC_PUBLIC)
	ZEND_ME(FbFormat, channelOrder, arginfo_class_FbFormat_channelOrder, ZEND_ACC_PUBLIC)
	ZEND_ME(FbFormat, scan, arginfo_class_FbFormat_scan, ZEND_ACC_PUBLIC)
	ZEND_ME(FbFormat, size, arginfo_class_FbFormat_size, ZEND_ACC_PUBLIC)
	ZEND_ME(FbFormat, mapRgba8, arginfo_class_FbFormat_mapRgba8, ZEND_ACC_PUBLIC)
	ZEND_ME(FbFormat, unmapRgba8, arginfo_class_FbFormat_unmapRgba8, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_FbFormat(void)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "FbFormat", class_FbFormat_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, ZEND_ACC_FINAL|ZEND_ACC_NOT_SERIALIZABLE);

	return class_entry;
}
