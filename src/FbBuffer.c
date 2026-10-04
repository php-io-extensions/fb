#include "runtime.h"
#include "../stubs/FbBuffer_arginfo.h"

zend_class_entry *phpfb_ce_FbBuffer;

static zend_object_handlers phpfb_buffer_handlers;

static zend_object *phpfb_buffer_create(zend_class_entry *ce)
{
	phpfb_buffer *self = zend_object_alloc(sizeof(phpfb_buffer), ce);

	zend_object_std_init(&self->std, ce);
	object_properties_init(&self->std, ce);
	ZVAL_UNDEF(&self->format_object);

	return &self->std;
}

static void phpfb_buffer_free(zend_object *obj)
{
	phpfb_buffer *self = phpfb_buffer_from(obj);

	if (self->bytes != NULL) {
		efree(self->bytes);
		self->bytes = NULL;
	}
	zval_ptr_dtor(&self->format_object);
	zend_object_std_dtor(obj);
}

void phpfb_register_FbBuffer(void)
{
	phpfb_ce_FbBuffer = register_class_FbBuffer();
	phpfb_ce_FbBuffer->create_object = phpfb_buffer_create;

	memcpy(&phpfb_buffer_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	phpfb_buffer_handlers.offset = XtOffsetOf(phpfb_buffer, std);
	phpfb_buffer_handlers.free_obj = phpfb_buffer_free;
	phpfb_buffer_handlers.clone_obj = NULL;
	phpfb_buffer_handlers.compare = zend_objects_not_comparable;
	phpfb_ce_FbBuffer->default_object_handlers = &phpfb_buffer_handlers;
}

#define PHPFB_THIS_BUFFER(self) \
	phpfb_buffer *self = phpfb_buffer_from(Z_OBJ_P(ZEND_THIS)); \
	if (UNEXPECTED(self->bytes == NULL)) { \
		zend_throw_error(NULL, "FbBuffer has not been constructed"); \
		RETURN_THROWS(); \
	}

static bool phpfb_inside(const phpfb_buffer *self, zend_long x, zend_long y)
{
	return x >= 0 && y >= 0 && x < self->width && y < self->height;
}

/* A rect argument that must be non-empty and inside the buffer. */
static bool phpfb_rect_inside(const phpfb_buffer *self, zend_long x, zend_long y, zend_long width, zend_long height)
{
	return x >= 0 && y >= 0 && width >= 1 && height >= 1 && width <= self->width - x && height <= self->height - y;
}

static void phpfb_return_rect(zval *return_value, fb_rect rect)
{
	array_init_size(return_value, 4);
	add_next_index_long(return_value, rect.x);
	add_next_index_long(return_value, rect.y);
	add_next_index_long(return_value, rect.width);
	add_next_index_long(return_value, rect.height);
}

/*
 * A pixel list entry: `arity` ints, the first two a coordinate inside the
 * buffer. Throws and answers false otherwise.
 */
static bool phpfb_read_pixel(const phpfb_buffer *self, zval *entry, uint32_t arity, zend_long position, zend_long *out)
{
	ZVAL_DEREF(entry);
	if (Z_TYPE_P(entry) != IS_ARRAY || zend_hash_num_elements(Z_ARRVAL_P(entry)) != arity) {
		goto malformed;
	}
	for (uint32_t i = 0; i < arity; i++) {
		zval *field = zend_hash_index_find(Z_ARRVAL_P(entry), i);
		if (field == NULL) {
			goto malformed;
		}
		ZVAL_DEREF(field);
		if (Z_TYPE_P(field) != IS_LONG) {
			goto malformed;
		}
		out[i] = Z_LVAL_P(field);
	}
	if (!phpfb_inside(self, out[0], out[1])) {
		zend_value_error("(" ZEND_LONG_FMT ", " ZEND_LONG_FMT ") is outside a %dx%d framebuffer.", out[0], out[1], self->width, self->height);
		return false;
	}

	return true;

malformed:
	zend_value_error("Pixel " ZEND_LONG_FMT " is not %s.", position, arity == 3 ? "[x, y, value]" : "[x, y]");
	return false;
}

/*
 * Write a pixel list: every entry is checked before anything is written.
 * `value` is used when the entries carry none (arity 2).
 */
static void phpfb_write_list(INTERNAL_FUNCTION_PARAMETERS, phpfb_buffer *self, HashTable *list, uint32_t arity, zend_long value)
{
	zval *entry;
	zend_long fields[3];
	zend_long position = 0;
	zend_long left = ZEND_LONG_MAX, top = ZEND_LONG_MAX, right = ZEND_LONG_MIN, bottom = ZEND_LONG_MIN;

	(void) execute_data;

	ZEND_HASH_FOREACH_VAL(list, entry) {
		if (!phpfb_read_pixel(self, entry, arity, position++, fields)) {
			RETURN_THROWS();
		}
		left = MIN(left, fields[0]);
		top = MIN(top, fields[1]);
		right = MAX(right, fields[0]);
		bottom = MAX(bottom, fields[1]);
	} ZEND_HASH_FOREACH_END();

	if (position == 0) {
		RETURN_NULL();
	}

	position = 0;
	ZEND_HASH_FOREACH_VAL(list, entry) {
		phpfb_read_pixel(self, entry, arity, position++, fields);
		fb_set(&self->format, self->bytes, self->width, self->height, (int) fields[0], (int) fields[1],
			(uint32_t) (arity == 3 ? fields[2] : value));
	} ZEND_HASH_FOREACH_END();

	phpfb_return_rect(return_value, (fb_rect) {(int) left, (int) top, (int) (right - left + 1), (int) (bottom - top + 1)});
}

ZEND_METHOD(FbBuffer, __construct)
{
	zend_object *format;
	zend_long width, height;

	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_OBJ_OF_CLASS(format, phpfb_ce_FbFormat)
		Z_PARAM_LONG(width)
		Z_PARAM_LONG(height)
	ZEND_PARSE_PARAMETERS_END();

	phpfb_buffer *self = phpfb_buffer_from(Z_OBJ_P(ZEND_THIS));
	if (self->bytes != NULL) {
		zend_throw_error(NULL, "FbBuffer is already constructed");
		RETURN_THROWS();
	}
	if (!phpfb_format_from(format)->resolved) {
		zend_throw_error(NULL, "FbFormat has not been constructed");
		RETURN_THROWS();
	}
	if (width < 1 || width > FB_MAX_SIDE) {
		zend_argument_value_error(2, "must be between 1 and %d", FB_MAX_SIDE);
		RETURN_THROWS();
	}
	if (height < 1 || height > FB_MAX_SIDE) {
		zend_argument_value_error(3, "must be between 1 and %d", FB_MAX_SIDE);
		RETURN_THROWS();
	}

	self->format = phpfb_format_from(format)->format;
	ZVAL_OBJ_COPY(&self->format_object, format);
	self->width = (int) width;
	self->height = (int) height;
	self->size = fb_size(&self->format, self->width, self->height);
	self->bytes = emalloc(self->size);
	fb_blank(&self->format, self->bytes, self->width, self->height);
}

ZEND_METHOD(FbBuffer, format)
{
	ZEND_PARSE_PARAMETERS_NONE();
	PHPFB_THIS_BUFFER(self);

	RETURN_COPY(&self->format_object);
}

ZEND_METHOD(FbBuffer, width)
{
	ZEND_PARSE_PARAMETERS_NONE();
	PHPFB_THIS_BUFFER(self);

	RETURN_LONG(self->width);
}

ZEND_METHOD(FbBuffer, height)
{
	ZEND_PARSE_PARAMETERS_NONE();
	PHPFB_THIS_BUFFER(self);

	RETURN_LONG(self->height);
}

ZEND_METHOD(FbBuffer, size)
{
	ZEND_PARSE_PARAMETERS_NONE();
	PHPFB_THIS_BUFFER(self);

	RETURN_LONG((zend_long) self->size);
}

ZEND_METHOD(FbBuffer, get)
{
	zend_long x, y;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_LONG(x)
		Z_PARAM_LONG(y)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	if (!phpfb_inside(self, x, y)) {
		zend_value_error("(" ZEND_LONG_FMT ", " ZEND_LONG_FMT ") is outside a %dx%d framebuffer.", x, y, self->width, self->height);
		RETURN_THROWS();
	}

	RETURN_LONG((zend_long) fb_get(&self->format, self->bytes, self->width, self->height, (int) x, (int) y));
}

ZEND_METHOD(FbBuffer, set)
{
	zend_long x, y, value;

	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_LONG(x)
		Z_PARAM_LONG(y)
		Z_PARAM_LONG(value)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	if (!phpfb_inside(self, x, y)) {
		zend_value_error("(" ZEND_LONG_FMT ", " ZEND_LONG_FMT ") is outside a %dx%d framebuffer.", x, y, self->width, self->height);
		RETURN_THROWS();
	}

	fb_set(&self->format, self->bytes, self->width, self->height, (int) x, (int) y, (uint32_t) value);
}

ZEND_METHOD(FbBuffer, setPixels)
{
	HashTable *pixels;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ARRAY_HT(pixels)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	phpfb_write_list(INTERNAL_FUNCTION_PARAM_PASSTHRU, self, pixels, 3, 0);
}

ZEND_METHOD(FbBuffer, setCoordinates)
{
	HashTable *coordinates;
	zend_long value;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_ARRAY_HT(coordinates)
		Z_PARAM_LONG(value)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	phpfb_write_list(INTERNAL_FUNCTION_PARAM_PASSTHRU, self, coordinates, 2, value);
}

ZEND_METHOD(FbBuffer, rect)
{
	zend_long x, y, width, height, value;

	ZEND_PARSE_PARAMETERS_START(5, 5)
		Z_PARAM_LONG(x)
		Z_PARAM_LONG(y)
		Z_PARAM_LONG(width)
		Z_PARAM_LONG(height)
		Z_PARAM_LONG(value)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	if (!phpfb_rect_inside(self, x, y, width, height)) {
		zend_value_error("The region " ZEND_LONG_FMT "x" ZEND_LONG_FMT " at (" ZEND_LONG_FMT ", " ZEND_LONG_FMT ") is empty or not inside a %dx%d framebuffer.",
			width, height, x, y, self->width, self->height);
		RETURN_THROWS();
	}

	fb_fill_rect(&self->format, self->bytes, self->width, self->height, (fb_rect) {(int) x, (int) y, (int) width, (int) height}, (uint32_t) value);
}

ZEND_METHOD(FbBuffer, fill)
{
	zend_long value;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(value)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	fb_fill_rect(&self->format, self->bytes, self->width, self->height, (fb_rect) {0, 0, self->width, self->height}, (uint32_t) value);
}

ZEND_METHOD(FbBuffer, bytes)
{
	ZEND_PARSE_PARAMETERS_NONE();
	PHPFB_THIS_BUFFER(self);

	RETURN_STRINGL((const char *) self->bytes, self->size);
}

ZEND_METHOD(FbBuffer, layer)
{
	zend_long layer;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(layer)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	int layers = self->format.layout == FB_LAYOUT_PLANAR ? self->format.ink_count : 1;
	if (layer < 0 || layer >= layers) {
		zend_value_error("Layer " ZEND_LONG_FMT " is outside 0..%d.", layer, layers - 1);
		RETURN_THROWS();
	}

	size_t plane = self->size / (size_t) layers;
	RETURN_STRINGL((const char *) self->bytes + plane * (size_t) layer, plane);
}

ZEND_METHOD(FbBuffer, region)
{
	zend_long x, y, width, height;
	zend_object *format = NULL;

	ZEND_PARSE_PARAMETERS_START(4, 5)
		Z_PARAM_LONG(x)
		Z_PARAM_LONG(y)
		Z_PARAM_LONG(width)
		Z_PARAM_LONG(height)
		Z_PARAM_OPTIONAL
		Z_PARAM_OBJ_OF_CLASS_OR_NULL(format, phpfb_ce_FbFormat)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	if (!phpfb_rect_inside(self, x, y, width, height)) {
		zend_value_error("The region " ZEND_LONG_FMT "x" ZEND_LONG_FMT " at (" ZEND_LONG_FMT ", " ZEND_LONG_FMT ") is empty or not inside a %dx%d framebuffer.",
			width, height, x, y, self->width, self->height);
		RETURN_THROWS();
	}
	if (format != NULL && !phpfb_format_from(format)->resolved) {
		zend_throw_error(NULL, "FbFormat has not been constructed");
		RETURN_THROWS();
	}

	const fb_format *to = format != NULL ? &phpfb_format_from(format)->format : &self->format;
	zend_string *out = zend_string_alloc(fb_size(to, (int) width, (int) height), 0);

	fb_region(&self->format, self->bytes, self->width, self->height, (fb_rect) {(int) x, (int) y, (int) width, (int) height}, to, (uint8_t *) ZSTR_VAL(out));
	ZSTR_VAL(out)[ZSTR_LEN(out)] = '\0';

	RETURN_NEW_STR(out);
}

ZEND_METHOD(FbBuffer, rgba8)
{
	ZEND_PARSE_PARAMETERS_NONE();
	PHPFB_THIS_BUFFER(self);

	zend_string *out = zend_string_alloc((size_t) self->width * (size_t) self->height * 4, 0);

	fb_rgba8(&self->format, self->bytes, self->width, self->height, (uint8_t *) ZSTR_VAL(out));
	ZSTR_VAL(out)[ZSTR_LEN(out)] = '\0';

	RETURN_NEW_STR(out);
}

ZEND_METHOD(FbBuffer, blitRgba8)
{
	zend_string *rgba8;
	zend_long width, height, x, y;
	zend_long clip_x = 0, clip_y = 0, clip_width = 0, clip_height = 0;
	bool clip_width_null = true, clip_height_null = true;

	ZEND_PARSE_PARAMETERS_START(5, 9)
		Z_PARAM_STR(rgba8)
		Z_PARAM_LONG(width)
		Z_PARAM_LONG(height)
		Z_PARAM_LONG(x)
		Z_PARAM_LONG(y)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(clip_x)
		Z_PARAM_LONG(clip_y)
		Z_PARAM_LONG_OR_NULL(clip_width, clip_width_null)
		Z_PARAM_LONG_OR_NULL(clip_height, clip_height_null)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	if (width < 1 || width > FB_MAX_SIDE) {
		zend_argument_value_error(2, "must be between 1 and %d", FB_MAX_SIDE);
		RETURN_THROWS();
	}
	if (height < 1 || height > FB_MAX_SIDE) {
		zend_argument_value_error(3, "must be between 1 and %d", FB_MAX_SIDE);
		RETURN_THROWS();
	}
	if (ZSTR_LEN(rgba8) != (size_t) width * (size_t) height * 4) {
		zend_argument_value_error(1, "must be " ZEND_LONG_FMT "x" ZEND_LONG_FMT " RGBA8 pixels (%zu bytes), %zu given",
			width, height, (size_t) width * (size_t) height * 4, ZSTR_LEN(rgba8));
		RETURN_THROWS();
	}

	/* The target: the source rect, clipped to the buffer and to the clip rect. */
	zend_long left = MAX(MAX(x, 0), clip_x);
	zend_long top = MAX(MAX(y, 0), clip_y);
	zend_long right = MIN(x + width, self->width);
	zend_long bottom = MIN(y + height, self->height);

	if (!clip_width_null) {
		right = clip_width > 0 ? MIN(right, clip_x + MIN(clip_width, (zend_long) FB_MAX_SIDE)) : left;
	}
	if (!clip_height_null) {
		bottom = clip_height > 0 ? MIN(bottom, clip_y + MIN(clip_height, (zend_long) FB_MAX_SIDE)) : top;
	}
	if (right <= left || bottom <= top) {
		RETURN_NULL();
	}

	fb_rect target = {(int) left, (int) top, (int) (right - left), (int) (bottom - top)};

	fb_blit_rgba8(&self->format, self->bytes, self->width, self->height, (const uint8_t *) ZSTR_VAL(rgba8), (int) width, (int) x, (int) y, target);
	phpfb_return_rect(return_value, target);
}

ZEND_METHOD(FbBuffer, copy)
{
	zend_object *source_object;
	zend_long x, y, width, height;

	ZEND_PARSE_PARAMETERS_START(5, 5)
		Z_PARAM_OBJ_OF_CLASS(source_object, phpfb_ce_FbBuffer)
		Z_PARAM_LONG(x)
		Z_PARAM_LONG(y)
		Z_PARAM_LONG(width)
		Z_PARAM_LONG(height)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	phpfb_buffer *source = phpfb_buffer_from(source_object);
	if (source->bytes == NULL) {
		zend_throw_error(NULL, "FbBuffer has not been constructed");
		RETURN_THROWS();
	}
	if (source->width != self->width || source->height != self->height || !fb_format_same_storage(&source->format, &self->format)) {
		zend_argument_value_error(1, "must be a buffer of the same size and storage");
		RETURN_THROWS();
	}
	if (!phpfb_rect_inside(self, x, y, width, height)) {
		zend_value_error("The region " ZEND_LONG_FMT "x" ZEND_LONG_FMT " at (" ZEND_LONG_FMT ", " ZEND_LONG_FMT ") is empty or not inside a %dx%d framebuffer.",
			width, height, x, y, self->width, self->height);
		RETURN_THROWS();
	}

	fb_copy(&self->format, self->bytes, source->bytes, self->width, self->height, (fb_rect) {(int) x, (int) y, (int) width, (int) height});
}

ZEND_METHOD(FbBuffer, plane)
{
	zend_long value;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(value)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	zend_string *out = zend_string_alloc(fb_plane_size(self->width, self->height), 0);

	if (value < 0 || value > UINT32_MAX) {
		/* No stored word is outside 32 bits, so no pixel matches. */
		memset(ZSTR_VAL(out), 0, ZSTR_LEN(out));
	} else {
		fb_plane(&self->format, self->bytes, self->width, self->height, (uint32_t) value, (uint8_t *) ZSTR_VAL(out));
	}
	ZSTR_VAL(out)[ZSTR_LEN(out)] = '\0';

	RETURN_NEW_STR(out);
}

ZEND_METHOD(FbBuffer, paintSpans)
{
	zend_string *spans;
	zend_long rgba;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_STR(spans)
		Z_PARAM_LONG(rgba)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	if (rgba < 0 || rgba > (zend_long) UINT32_MAX) {
		zend_value_error("paintSpans() takes a colour 0xRRGGBBAA, got " ZEND_LONG_FMT ".", rgba);
		RETURN_THROWS();
	}
	if (ZSTR_LEN(spans) % FB_SPAN_BYTES != 0) {
		zend_value_error("A span list is a whole number of %d-byte spans, got %zu bytes.", FB_SPAN_BYTES, ZSTR_LEN(spans));
		RETURN_THROWS();
	}

	/* Every span is checked before any is painted. */
	size_t count = ZSTR_LEN(spans) / FB_SPAN_BYTES;
	const uint8_t *bytes = (const uint8_t *) ZSTR_VAL(spans);
	int left = INT_MAX, top = INT_MAX, right = INT_MIN, bottom = INT_MIN;
	for (size_t i = 0; i < count; i++) {
		const uint8_t *p = bytes + i * FB_SPAN_BYTES;
		int y = p[0] | (p[1] << 8), x = p[2] | (p[3] << 8), length = p[4] | (p[5] << 8);

		if (length < 1 || y >= self->height || length > self->width - x) {
			zend_value_error("Span %zu is empty or not inside a %dx%d framebuffer.", i, self->width, self->height);
			RETURN_THROWS();
		}
		left = MIN(left, x);
		top = MIN(top, y);
		right = MAX(right, x + length);
		bottom = MAX(bottom, y + 1);
	}
	if (count == 0) {
		RETURN_NULL();
	}

	fb_paint_spans(&self->format, self->bytes, self->width, self->height, bytes, count, (uint32_t) rgba);
	phpfb_return_rect(return_value, (fb_rect) {left, top, right - left, bottom - top});
}

ZEND_METHOD(FbBuffer, pointer)
{
	ZEND_PARSE_PARAMETERS_NONE();
	PHPFB_THIS_BUFFER(self);

	RETURN_LONG((zend_long) (uintptr_t) self->bytes);
}

ZEND_METHOD(FbBuffer, granularity)
{
	ZEND_PARSE_PARAMETERS_NONE();
	PHPFB_THIS_BUFFER(self);

	int unit_width, unit_height;

	fb_granularity(&self->format, self->width, &unit_width, &unit_height);
	array_init_size(return_value, 2);
	add_next_index_long(return_value, unit_width);
	add_next_index_long(return_value, unit_height);
}
