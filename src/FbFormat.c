#include "runtime.h"
#include "../stubs/FbFormat_arginfo.h"

zend_class_entry *phpfb_ce_FbFormat;

static zend_object_handlers phpfb_format_handlers;

static zend_object *phpfb_format_create(zend_class_entry *ce)
{
	phpfb_format *self = zend_object_alloc(sizeof(phpfb_format), ce);

	zend_object_std_init(&self->std, ce);
	object_properties_init(&self->std, ce);

	return &self->std;
}

void phpfb_register_FbFormat(void)
{
	phpfb_ce_FbFormat = register_class_FbFormat();
	phpfb_ce_FbFormat->create_object = phpfb_format_create;

	memcpy(&phpfb_format_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	phpfb_format_handlers.offset = XtOffsetOf(phpfb_format, std);
	phpfb_format_handlers.clone_obj = NULL;
	phpfb_format_handlers.compare = zend_objects_not_comparable;
	phpfb_ce_FbFormat->default_object_handlers = &phpfb_format_handlers;
}

#define PHPFB_THIS_FORMAT(self) \
	phpfb_format *self = phpfb_format_from(Z_OBJ_P(ZEND_THIS)); \
	if (UNEXPECTED(!self->resolved)) { \
		zend_throw_error(NULL, "FbFormat has not been constructed"); \
		RETURN_THROWS(); \
	}

/* One palette entry: [int rgb, bool inverted, int code]. */
static bool phpfb_read_ink(zval *entry, fb_ink *ink)
{
	ZVAL_DEREF(entry);
	if (Z_TYPE_P(entry) != IS_ARRAY || zend_hash_num_elements(Z_ARRVAL_P(entry)) != 3) {
		return false;
	}

	zval *rgb = zend_hash_index_find(Z_ARRVAL_P(entry), 0);
	zval *inverted = zend_hash_index_find(Z_ARRVAL_P(entry), 1);
	zval *code = zend_hash_index_find(Z_ARRVAL_P(entry), 2);
	if (rgb == NULL || inverted == NULL || code == NULL) {
		return false;
	}
	ZVAL_DEREF(rgb);
	ZVAL_DEREF(inverted);
	ZVAL_DEREF(code);
	if (Z_TYPE_P(rgb) != IS_LONG || (Z_TYPE_P(inverted) != IS_TRUE && Z_TYPE_P(inverted) != IS_FALSE) || Z_TYPE_P(code) != IS_LONG) {
		return false;
	}
	if (Z_LVAL_P(rgb) < 0 || Z_LVAL_P(rgb) > 0xFFFFFF || Z_LVAL_P(code) < 0 || Z_LVAL_P(code) > 0xFFFF) {
		return false;
	}

	ink->rgb = (uint32_t) Z_LVAL_P(rgb);
	ink->inverted = Z_TYPE_P(inverted) == IS_TRUE;
	ink->code = (uint32_t) Z_LVAL_P(code);

	return true;
}

ZEND_METHOD(FbFormat, __construct)
{
	zend_long layout;
	zend_long bit_order = FB_BIT_ORDER_MSB;
	zend_long byte_order = FB_BYTE_ORDER_MSB;
	zend_long channel_order = FB_CHANNELS_RGB;
	zend_long scan = FB_SCAN_TOP_DOWN;
	HashTable *palette = NULL;

	ZEND_PARSE_PARAMETERS_START(1, 6)
		Z_PARAM_LONG(layout)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(bit_order)
		Z_PARAM_LONG(byte_order)
		Z_PARAM_LONG(channel_order)
		Z_PARAM_LONG(scan)
		Z_PARAM_ARRAY_HT(palette)
	ZEND_PARSE_PARAMETERS_END();

	phpfb_format *self = phpfb_format_from(Z_OBJ_P(ZEND_THIS));
	if (self->resolved) {
		zend_throw_error(NULL, "FbFormat is already constructed");
		RETURN_THROWS();
	}

	fb_format format = {0};
	format.layout = phpfb_narrow(layout);
	format.bit_order = phpfb_narrow(bit_order);
	format.byte_order = phpfb_narrow(byte_order);
	format.channel_order = phpfb_narrow(channel_order);
	format.scan = phpfb_narrow(scan);

	if (palette != NULL) {
		zval *entry;

		if (zend_hash_num_elements(palette) > FB_MAX_INKS) {
			zend_argument_value_error(6, "must hold at most %d inks", FB_MAX_INKS);
			RETURN_THROWS();
		}
		ZEND_HASH_FOREACH_VAL(palette, entry) {
			if (!phpfb_read_ink(entry, &format.inks[format.ink_count])) {
				zend_argument_value_error(6, "must hold [int $rgb, bool $inverted, int $code] entries, with $rgb in 0..0xFFFFFF and $code in 0..0xFFFF");
				RETURN_THROWS();
			}
			format.ink_count++;
		} ZEND_HASH_FOREACH_END();
	}

	const char *error = fb_format_resolve(&format);
	if (error != NULL) {
		zend_value_error("FbFormat::__construct(): %s", error);
		RETURN_THROWS();
	}

	self->format = format;
	self->resolved = true;
}

ZEND_METHOD(FbFormat, layout)
{
	ZEND_PARSE_PARAMETERS_NONE();
	PHPFB_THIS_FORMAT(self);

	RETURN_LONG(self->format.layout);
}

ZEND_METHOD(FbFormat, bitOrder)
{
	ZEND_PARSE_PARAMETERS_NONE();
	PHPFB_THIS_FORMAT(self);

	RETURN_LONG(self->format.bit_order);
}

ZEND_METHOD(FbFormat, byteOrder)
{
	ZEND_PARSE_PARAMETERS_NONE();
	PHPFB_THIS_FORMAT(self);

	RETURN_LONG(self->format.byte_order);
}

ZEND_METHOD(FbFormat, channelOrder)
{
	ZEND_PARSE_PARAMETERS_NONE();
	PHPFB_THIS_FORMAT(self);

	RETURN_LONG(self->format.channel_order);
}

ZEND_METHOD(FbFormat, scan)
{
	ZEND_PARSE_PARAMETERS_NONE();
	PHPFB_THIS_FORMAT(self);

	RETURN_LONG(self->format.scan);
}

ZEND_METHOD(FbFormat, size)
{
	zend_long width, height;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_LONG(width)
		Z_PARAM_LONG(height)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_FORMAT(self);

	if (width < 1 || width > FB_MAX_SIDE) {
		zend_argument_value_error(1, "must be between 1 and %d", FB_MAX_SIDE);
		RETURN_THROWS();
	}
	if (height < 1 || height > FB_MAX_SIDE) {
		zend_argument_value_error(2, "must be between 1 and %d", FB_MAX_SIDE);
		RETURN_THROWS();
	}

	RETURN_LONG((zend_long) fb_size(&self->format, (int) width, (int) height));
}

ZEND_METHOD(FbFormat, mapRgba8)
{
	zend_long channels[4] = {0, 0, 0, 255};

	ZEND_PARSE_PARAMETERS_START(3, 4)
		Z_PARAM_LONG(channels[0])
		Z_PARAM_LONG(channels[1])
		Z_PARAM_LONG(channels[2])
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(channels[3])
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_FORMAT(self);

	for (uint32_t i = 0; i < 4; i++) {
		if (channels[i] < 0 || channels[i] > 255) {
			zend_argument_value_error(i + 1, "must be between 0 and 255");
			RETURN_THROWS();
		}
	}

	RETURN_LONG((zend_long) fb_map(&self->format, (int) channels[0], (int) channels[1], (int) channels[2], (int) channels[3]));
}

ZEND_METHOD(FbFormat, unmapRgba8)
{
	zend_long word;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(word)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_FORMAT(self);

	RETURN_LONG((zend_long) fb_unmap(&self->format, (uint32_t) word));
}
