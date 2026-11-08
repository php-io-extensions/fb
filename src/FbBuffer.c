#include "runtime.h"
#include "sink.h"
#include "filter.h"
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
	if (!phpfb_addressable(width, height)) {
		zend_value_error("A " ZEND_LONG_FMT "x" ZEND_LONG_FMT " framebuffer is too large to address on this platform.", width, height);
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
	if ((uint64_t) ZSTR_LEN(rgba8) != (uint64_t) width * (uint64_t) height * 4u) {
		zend_argument_value_error(1, "must be " ZEND_LONG_FMT "x" ZEND_LONG_FMT " RGBA8 pixels (%" PRIu64 " bytes), %zu given",
			width, height, (uint64_t) width * (uint64_t) height * 4u, ZSTR_LEN(rgba8));
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

/* Spans checked, every one before any is painted, then painted: 1 with their bounds, 0 for none, -1 with a ValueError. */
static int phpfb_spans_paint(phpfb_buffer *self, const uint8_t *bytes, size_t count, uint32_t rgba, fb_rect *bounds)
{
	int left = INT_MAX, top = INT_MAX, right = INT_MIN, bottom = INT_MIN;

	for (size_t i = 0; i < count; i++) {
		const uint8_t *p = bytes + i * FB_SPAN_BYTES;
		int y = p[0] | (p[1] << 8), x = p[2] | (p[3] << 8), length = p[4] | (p[5] << 8);

		if (length < 1 || y >= self->height || length > self->width - x) {
			zend_value_error("Span %zu is empty or not inside a %dx%d framebuffer.", i, self->width, self->height);
			return -1;
		}
		left = MIN(left, x);
		top = MIN(top, y);
		right = MAX(right, x + length);
		bottom = MAX(bottom, y + 1);
	}
	if (count == 0) {
		return 0;
	}

	fb_paint_spans(&self->format, self->bytes, self->width, self->height, bytes, count, rgba);
	*bounds = (fb_rect) {left, top, right - left, bottom - top};

	return 1;
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

	fb_rect bounds;
	int painted = phpfb_spans_paint(self, (const uint8_t *) ZSTR_VAL(spans), ZSTR_LEN(spans) / FB_SPAN_BYTES, (uint32_t) rgba, &bounds);
	if (painted < 0) {
		RETURN_THROWS();
	}
	if (painted == 0) {
		RETURN_NULL();
	}
	phpfb_return_rect(return_value, bounds);
}

/* The span sink (src/sink.h): an FbBuffer painted from C by another extension. */
static phpfb_buffer *phpfb_sink_buffer(zend_object *object)
{
	if (object->ce != phpfb_ce_FbBuffer) {
		return NULL;
	}
	phpfb_buffer *self = phpfb_buffer_from(object);

	return self->bytes != NULL ? self : NULL;
}

static bool phpfb_sink_size(zend_object *object, int *width, int *height)
{
	phpfb_buffer *self = phpfb_sink_buffer(object);

	if (self == NULL) {
		return false;
	}
	*width = self->width;
	*height = self->height;

	return true;
}

static int phpfb_sink_paint_spans(zend_object *object, const uint8_t *spans, size_t count, uint32_t rgba, int bounds[4])
{
	phpfb_buffer *self = phpfb_sink_buffer(object);
	fb_rect rect;

	if (self == NULL) {
		zend_value_error("The span sink paints into a constructed FbBuffer.");
		return -1;
	}
	int painted = phpfb_spans_paint(self, spans, count, rgba, &rect);
	if (painted == 1) {
		bounds[0] = rect.x;
		bounds[1] = rect.y;
		bounds[2] = rect.width;
		bounds[3] = rect.height;
	}

	return painted;
}

ZEND_DLEXPORT const phpfb_span_sink_t phpfb_span_sink = {PHPFB_SINK_VERSION, phpfb_sink_size, phpfb_sink_paint_spans};

/* A mask's place may lie off the buffer (the clip cuts it), but within this, so its edges stay in an int. */
#define PHPFB_MASK_REACH ((zend_long) 1 << 30)

ZEND_METHOD(FbBuffer, paintMasks)
{
	HashTable *list;
	zend_long rgba, clip_x, clip_y, clip_width, clip_height;

	ZEND_PARSE_PARAMETERS_START(6, 6)
		Z_PARAM_ARRAY_HT(list)
		Z_PARAM_LONG(rgba)
		Z_PARAM_LONG(clip_x)
		Z_PARAM_LONG(clip_y)
		Z_PARAM_LONG(clip_width)
		Z_PARAM_LONG(clip_height)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	if (rgba < 0 || rgba > (zend_long) UINT32_MAX) {
		zend_value_error("paintMasks() takes a colour 0xRRGGBBAA, got " ZEND_LONG_FMT ".", rgba);
		RETURN_THROWS();
	}
	if (!phpfb_rect_inside(self, clip_x, clip_y, clip_width, clip_height)) {
		zend_value_error("The clip is empty or not inside a %dx%d framebuffer.", self->width, self->height);
		RETURN_THROWS();
	}

	/* Every mask is checked before any is painted. */
	uint32_t count = zend_hash_num_elements(list);
	fb_mask *masks = count == 0 ? NULL : safe_emalloc(count, sizeof(fb_mask), 0);
	uint32_t i = 0;
	zval *entry;
	ZEND_HASH_FOREACH_VAL(list, entry) {
		zval *coverage, *width, *left, *top;
		HashTable *m = Z_TYPE_P(entry) == IS_ARRAY ? Z_ARRVAL_P(entry) : NULL;

		if (m == NULL || zend_hash_num_elements(m) != 4
			|| (coverage = zend_hash_index_find(m, 0)) == NULL || Z_TYPE_P(coverage) != IS_STRING
			|| (width = zend_hash_index_find(m, 1)) == NULL || Z_TYPE_P(width) != IS_LONG
			|| (left = zend_hash_index_find(m, 2)) == NULL || Z_TYPE_P(left) != IS_LONG
			|| (top = zend_hash_index_find(m, 3)) == NULL || Z_TYPE_P(top) != IS_LONG) {
			efree(masks);
			zend_value_error("Mask %u is not [string coverage, int width, int left, int top].", i);
			RETURN_THROWS();
		}
		size_t bytes = Z_STRLEN_P(coverage);
		if (Z_LVAL_P(width) < 1 || Z_LVAL_P(width) > FB_MAX_SIDE || bytes == 0 || bytes % (size_t) Z_LVAL_P(width) != 0
			|| bytes / (size_t) Z_LVAL_P(width) > FB_MAX_SIDE) {
			efree(masks);
			zend_value_error("Mask %u is not whole rows of 1..%d bytes, at most %d rows.", i, FB_MAX_SIDE, FB_MAX_SIDE);
			RETURN_THROWS();
		}
		if (Z_LVAL_P(left) < -PHPFB_MASK_REACH || Z_LVAL_P(left) > PHPFB_MASK_REACH || Z_LVAL_P(top) < -PHPFB_MASK_REACH || Z_LVAL_P(top) > PHPFB_MASK_REACH) {
			efree(masks);
			zend_value_error("Mask %u is placed past +/-2^30.", i);
			RETURN_THROWS();
		}
		masks[i++] = (fb_mask) {(const uint8_t *) Z_STRVAL_P(coverage), (int) Z_LVAL_P(width), (int) (bytes / (size_t) Z_LVAL_P(width)), (int) Z_LVAL_P(left), (int) Z_LVAL_P(top)};
	} ZEND_HASH_FOREACH_END();

	fb_rect bounds;
	int painted = count > 0 && fb_paint_masks(&self->format, self->bytes, self->width, self->height, masks, count, (uint32_t) rgba,
		(fb_rect) {(int) clip_x, (int) clip_y, (int) clip_width, (int) clip_height}, &bounds);
	if (masks != NULL) {
		efree(masks);
	}
	if (!painted) {
		RETURN_NULL();
	}
	phpfb_return_rect(return_value, bounds);
}

/* One element of a filter step, by index: a whole number, a finite number, a name (null or string), a bool. */
static bool phpfb_step_long(HashTable *step, uint32_t i, zend_long low, zend_long high, zend_long *out)
{
	zval *v = zend_hash_index_find(step, i);

	if (v == NULL || Z_TYPE_P(v) != IS_LONG || Z_LVAL_P(v) < low || Z_LVAL_P(v) > high) {
		return false;
	}
	*out = Z_LVAL_P(v);
	return true;
}

static bool phpfb_step_double(HashTable *step, uint32_t i, double *out)
{
	zval *v = zend_hash_index_find(step, i);

	if (v == NULL) {
		return false;
	}
	if (Z_TYPE_P(v) == IS_LONG) {
		*out = (double) Z_LVAL_P(v);
		return true;
	}
	if (Z_TYPE_P(v) == IS_DOUBLE && zend_finite(Z_DVAL_P(v))) {
		*out = Z_DVAL_P(v);
		return true;
	}
	return false;
}

static bool phpfb_step_name(HashTable *step, uint32_t i, const char **out)
{
	zval *v = zend_hash_index_find(step, i);

	if (v == NULL || (Z_TYPE_P(v) != IS_NULL && Z_TYPE_P(v) != IS_STRING)) {
		return false;
	}
	*out = Z_TYPE_P(v) == IS_STRING ? Z_STRVAL_P(v) : NULL;
	return true;
}

static bool phpfb_step_list(HashTable *step, uint32_t i, uint32_t count, double *out)
{
	zval *v = zend_hash_index_find(step, i), *item;
	uint32_t n = 0;

	if (v == NULL || Z_TYPE_P(v) != IS_ARRAY || !zend_array_is_list(Z_ARRVAL_P(v)) || zend_hash_num_elements(Z_ARRVAL_P(v)) != count) {
		return false;
	}
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(v), item) {
		ZVAL_DEREF(item);
		if (Z_TYPE_P(item) == IS_LONG) {
			out[n++] = (double) Z_LVAL_P(item);
		} else if (Z_TYPE_P(item) == IS_DOUBLE && zend_finite(Z_DVAL_P(item))) {
			out[n++] = Z_DVAL_P(item);
		} else {
			return false;
		}
	} ZEND_HASH_FOREACH_END();
	return true;
}

/* Parses step $index of the list into `s`; false (with a ValueError thrown) when it is malformed. */
static bool phpfb_parse_step(HashTable *step, zend_ulong index, HashTable *maps, fb_filter_step *s)
{
	zval *kind = zend_hash_index_find(step, 0);
	zend_long n, m;
	const char *name = NULL;

	memset(s, 0, sizeof(*s));
	if (kind == NULL || Z_TYPE_P(kind) != IS_STRING) {
		goto bad;
	}
#define FB_LONG(i, lo, hi, into) do { if (!phpfb_step_long(step, (i), (lo), (hi), &n)) goto bad; (into) = (__typeof__(into)) n; } while (0)
#define FB_DOUBLE(i, into) do { if (!phpfb_step_double(step, (i), &(into))) goto bad; } while (0)
	if (zend_string_equals_literal(Z_STR_P(kind), "blur")) {
		s->kind = FB_FILTER_BLUR;
		FB_LONG(1, 0, 1, s->sampling);
		FB_DOUBLE(2, s->sx);
		FB_DOUBLE(3, s->sy);
		FB_LONG(4, 1, 64, s->samples);
		FB_LONG(5, 0, 1, s->edge);
		if (!phpfb_step_name(step, 6, &s->in) || !phpfb_step_name(step, 7, &s->result)) goto bad;
		if (s->sx < 0.0 || s->sy < 0.0) goto bad;
	} else if (zend_string_equals_literal(Z_STR_P(kind), "matrix")) {
		s->kind = FB_FILTER_MATRIX;
		if (!phpfb_step_list(step, 1, 20, s->matrix) || !phpfb_step_name(step, 2, &s->in) || !phpfb_step_name(step, 3, &s->result)) goto bad;
	} else if (zend_string_equals_literal(Z_STR_P(kind), "flood")) {
		s->kind = FB_FILTER_FLOOD;
		for (int c = 0; c < 4; c++) {
			FB_LONG(1 + c, 0, 255, s->flood[c]);
		}
		if (!phpfb_step_name(step, 5, &s->result)) goto bad;
	} else if (zend_string_equals_literal(Z_STR_P(kind), "composite")) {
		s->kind = FB_FILTER_COMPOSITE;
		if (!phpfb_step_name(step, 1, &s->in) || !phpfb_step_name(step, 2, &s->in2)) goto bad;
		FB_LONG(3, 0, 5, s->op);
		for (int c = 0; c < 4; c++) {
			FB_DOUBLE(4 + c, s->k[c]);
		}
		if (!phpfb_step_name(step, 8, &s->result)) goto bad;
	} else if (zend_string_equals_literal(Z_STR_P(kind), "blend")) {
		s->kind = FB_FILTER_BLEND;
		if (!phpfb_step_name(step, 1, &s->in) || !phpfb_step_name(step, 2, &s->in2)) goto bad;
		FB_LONG(3, 0, 4, s->op);
		if (!phpfb_step_name(step, 4, &s->result)) goto bad;
	} else if (zend_string_equals_literal(Z_STR_P(kind), "noise")) {
		zval *stitch = zend_hash_index_find(step, 6);

		s->kind = FB_FILTER_NOISE;
		FB_LONG(1, 0, 2, s->noise);
		FB_DOUBLE(2, s->fx);
		FB_DOUBLE(3, s->fy);
		FB_LONG(4, 1, 4, s->octaves);
		FB_LONG(5, INT32_MIN, INT32_MAX, s->seed);
		if (stitch == NULL || (Z_TYPE_P(stitch) != IS_TRUE && Z_TYPE_P(stitch) != IS_FALSE)) goto bad;
		s->stitch = Z_TYPE_P(stitch) == IS_TRUE;
		if (!phpfb_step_name(step, 7, &s->result) || s->fx < 0.0 || s->fy < 0.0) goto bad;
	} else if (zend_string_equals_literal(Z_STR_P(kind), "displace")) {
		zval *map;

		s->kind = FB_FILTER_DISPLACE;
		if (!phpfb_step_name(step, 1, &s->in)) goto bad;
		{
			zval *which = zend_hash_index_find(step, 2);

			if (which != NULL && Z_TYPE_P(which) == IS_STRING) {
				/* the map is an earlier step's result */
				s->in2 = Z_STRVAL_P(which);
			} else {
				FB_LONG(2, 0, ZEND_LONG_MAX, m);
				map = zend_hash_index_find(maps, (zend_ulong) m);
				if (map == NULL || Z_TYPE_P(map) != IS_ARRAY) goto bad;
				{
					HashTable *entry = Z_ARRVAL_P(map);
					zval *bytes = zend_hash_index_find(entry, 2);
					zend_long w, h;

					if (!phpfb_step_long(entry, 0, 1, FB_MAX_SIDE, &w) || !phpfb_step_long(entry, 1, 1, FB_MAX_SIDE, &h)
						|| bytes == NULL || Z_TYPE_P(bytes) != IS_STRING || (uint64_t) Z_STRLEN_P(bytes) != (uint64_t) w * (uint64_t) h * 4u) goto bad;
					s->map = (const uint8_t *) Z_STRVAL_P(bytes);
					s->map_w = (int) w;
					s->map_h = (int) h;
				}
			}
		}
		FB_DOUBLE(3, s->scale);
		FB_LONG(4, 0, 3, s->xchannel);
		FB_LONG(5, 0, 3, s->ychannel);
		FB_LONG(6, 1, FB_FILTER_MAX_BANDS, s->bands);
		FB_DOUBLE(7, s->spread);
		if (s->bands > 1 && !phpfb_step_list(step, 8, (uint32_t) s->bands * 3, s->weights)) goto bad;
		if (!phpfb_step_name(step, 9, &s->result)) goto bad;
	} else if (zend_string_equals_literal(Z_STR_P(kind), "field")) {
		s->kind = FB_FILTER_FIELD;
		FB_LONG(1, 0, 2, s->field);
		for (int c = 0; c < 4; c++) {
			FB_DOUBLE(2 + c, s->box[c]);
			FB_DOUBLE(6 + c, s->radii[c]);
		}
		FB_DOUBLE(10, s->from);
		FB_DOUBLE(11, s->to);
		if (!phpfb_step_name(step, 12, &s->result) || (s->field != 1 && fabs(s->to - s->from) < 1e-30)) goto bad;
	} else if (zend_string_equals_literal(Z_STR_P(kind), "table")) {
		s->kind = FB_FILTER_TABLE;
		if (!phpfb_step_name(step, 1, &s->in) || !phpfb_step_name(step, 6, &s->result)) goto bad;
		for (int c = 0; c < 4; c++) {
			zval *t = zend_hash_index_find(step, 2 + c);

			if (t == NULL || (Z_TYPE_P(t) != IS_NULL && (Z_TYPE_P(t) != IS_STRING || Z_STRLEN_P(t) != 256))) goto bad;
			s->tables[c] = Z_TYPE_P(t) == IS_STRING ? (const uint8_t *) Z_STRVAL_P(t) : NULL;
		}
	} else {
		goto bad;
	}
#undef FB_LONG
#undef FB_DOUBLE
	(void) name;
	return true;

bad:
	zend_value_error("Filter step " ZEND_ULONG_FMT " is not a well-formed step.", index);
	return false;
}

/* A name a step reads must be "source" or the result of an earlier step. */
static bool phpfb_name_known(const fb_filter_step *steps, size_t before, const char *name)
{
	if (name == NULL || name[0] == '\0' || strcmp(name, "source") == 0) {
		return true;
	}
	for (size_t i = 0; i < before; i++) {
		if (steps[i].result != NULL && strcmp(steps[i].result, name) == 0) {
			return true;
		}
	}
	return false;
}

ZEND_METHOD(FbBuffer, filter)
{
	HashTable *step_list, *maps;
	zend_long x, y, width, height, row = 0;
	bool premultiplied = false;

	ZEND_PARSE_PARAMETERS_START(6, 8)
		Z_PARAM_ARRAY_HT(step_list)
		Z_PARAM_ARRAY_HT(maps)
		Z_PARAM_LONG(x)
		Z_PARAM_LONG(y)
		Z_PARAM_LONG(width)
		Z_PARAM_LONG(height)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(row)
		Z_PARAM_BOOL(premultiplied)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	if (!phpfb_rect_inside(self, x, y, width, height)) {
		zend_value_error("The region " ZEND_LONG_FMT "x" ZEND_LONG_FMT " at (" ZEND_LONG_FMT ", " ZEND_LONG_FMT ") is empty or not inside a %dx%d framebuffer.",
			width, height, x, y, self->width, self->height);
		RETURN_THROWS();
	}
	if (row < 0 || row > FB_MAX_SIDE) {
		zend_argument_value_error(7, "must be between 0 and %d", FB_MAX_SIDE);
		RETURN_THROWS();
	}
	if (!zend_array_is_list(step_list) || !zend_array_is_list(maps)) {
		zend_argument_value_error(1, "must be a list of steps, and the maps a list");
		RETURN_THROWS();
	}

	size_t count = zend_hash_num_elements(step_list), at = 0;
	fb_filter_step *steps = safe_emalloc(count ? count : 1, sizeof(fb_filter_step), 0);
	zval *entry;

	ZEND_HASH_FOREACH_VAL(step_list, entry) {
		ZVAL_DEREF(entry);
		if (Z_TYPE_P(entry) != IS_ARRAY || !phpfb_parse_step(Z_ARRVAL_P(entry), at, maps, &steps[at])) {
			if (!EG(exception)) {
				zend_value_error("Filter step %zu is not a well-formed step.", at);
			}
			efree(steps);
			RETURN_THROWS();
		}
		if (!phpfb_name_known(steps, at, steps[at].in) || !phpfb_name_known(steps, at, steps[at].in2)) {
			zend_value_error("Filter step %zu reads a name no earlier step made.", at);
			efree(steps);
			RETURN_THROWS();
		}
		at++;
	} ZEND_HASH_FOREACH_END();

	/* the region's pixels, straight RGBA8 */
	uint8_t *pixels = safe_emalloc((size_t) width * (size_t) height, 4, 0);
	for (zend_long j = 0; j < height; j++) {
		for (zend_long i = 0; i < width; i++) {
			uint32_t d = fb_unmap(&self->format, fb_get(&self->format, self->bytes, self->width, self->height, (int) (x + i), (int) (y + j)));
			uint8_t *p = pixels + ((size_t) j * (size_t) width + (size_t) i) * 4;

			p[0] = (uint8_t) (d >> 24);
			p[1] = (uint8_t) (d >> 16);
			p[2] = (uint8_t) (d >> 8);
			p[3] = (uint8_t) d;
		}
	}
	uint8_t *result = fb_filter_run(steps, count, pixels, (int) width, (int) height, (int) x, (int) (y + row), premultiplied);

	efree(steps);
	efree(pixels);
	if (result == NULL) {
		zend_throw_error(NULL, "filter() ran out of memory.");
		RETURN_THROWS();
	}
	fb_blit_rgba8(&self->format, self->bytes, self->width, self->height, result, (int) width, (int) x, (int) y, (fb_rect) {(int) x, (int) y, (int) width, (int) height});
	free(result);
}

ZEND_METHOD(FbBuffer, paintGradient)
{
	zend_string *spans, *ramp;
	bool radial;
	zend_long spread, row = 0;
	HashTable *inverse_list;

	ZEND_PARSE_PARAMETERS_START(5, 6)
		Z_PARAM_STR(spans)
		Z_PARAM_STR(ramp)
		Z_PARAM_BOOL(radial)
		Z_PARAM_LONG(spread)
		Z_PARAM_ARRAY_HT(inverse_list)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(row)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	if (ZSTR_LEN(ramp) != 1024) {
		zend_argument_value_error(2, "must be 256 RGBA8 colours (1024 bytes), %zu given", ZSTR_LEN(ramp));
		RETURN_THROWS();
	}
	if (spread < 0 || spread > 2) {
		zend_argument_value_error(4, "must be 0 (pad), 1 (repeat) or 2 (reflect)");
		RETURN_THROWS();
	}
	if (row < 0 || row > FB_MAX_SIDE) {
		zend_argument_value_error(6, "must be between 0 and %d", FB_MAX_SIDE);
		RETURN_THROWS();
	}
	double inverse[6];
	uint32_t number = 0;
	zval *entry;
	if (!zend_array_is_list(inverse_list) || zend_hash_num_elements(inverse_list) != 6) {
		zend_argument_value_error(5, "must be a list of six finite numbers");
		RETURN_THROWS();
	}
	ZEND_HASH_FOREACH_VAL(inverse_list, entry) {
		ZVAL_DEREF(entry);
		if (Z_TYPE_P(entry) == IS_LONG) {
			inverse[number] = (double) Z_LVAL_P(entry);
		} else if (Z_TYPE_P(entry) == IS_DOUBLE && zend_finite(Z_DVAL_P(entry))) {
			inverse[number] = Z_DVAL_P(entry);
		} else {
			zend_argument_value_error(5, "must be a list of six finite numbers");
			RETURN_THROWS();
		}
		number++;
	} ZEND_HASH_FOREACH_END();
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

	fb_paint_gradient(&self->format, self->bytes, self->width, self->height, bytes, count, (const uint8_t *) ZSTR_VAL(ramp), radial, (int) spread, inverse, (int) row);
	phpfb_return_rect(return_value, (fb_rect) {left, top, right - left, bottom - top});
}

ZEND_METHOD(FbBuffer, paintRgba8)
{
	zend_string *rgba8;
	HashTable *inverse_list;
	zend_long width, height, x, y, target_width, target_height, opacity = 255, row = 0, tint = (zend_long) UINT32_MAX;
	bool smooth = false;
	HashTable *window_list = NULL;
	zend_string *coverage = NULL;

	ZEND_PARSE_PARAMETERS_START(8, 14)
		Z_PARAM_STR(rgba8)
		Z_PARAM_LONG(width)
		Z_PARAM_LONG(height)
		Z_PARAM_ARRAY_HT(inverse_list)
		Z_PARAM_LONG(x)
		Z_PARAM_LONG(y)
		Z_PARAM_LONG(target_width)
		Z_PARAM_LONG(target_height)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(opacity)
		Z_PARAM_BOOL(smooth)
		Z_PARAM_LONG(row)
		Z_PARAM_LONG(tint)
		Z_PARAM_ARRAY_HT_OR_NULL(window_list)
		Z_PARAM_STR_OR_NULL(coverage)
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
	if ((uint64_t) ZSTR_LEN(rgba8) != (uint64_t) width * (uint64_t) height * 4u) {
		zend_argument_value_error(1, "must be " ZEND_LONG_FMT "x" ZEND_LONG_FMT " RGBA8 pixels (%" PRIu64 " bytes), %zu given",
			width, height, (uint64_t) width * (uint64_t) height * 4u, ZSTR_LEN(rgba8));
		RETURN_THROWS();
	}

	double inverse[6];
	uint32_t count = 0;
	zval *entry;
	if (!zend_array_is_list(inverse_list) || zend_hash_num_elements(inverse_list) != 6) {
		zend_argument_value_error(4, "must be a list of six finite numbers");
		RETURN_THROWS();
	}
	ZEND_HASH_FOREACH_VAL(inverse_list, entry) {
		ZVAL_DEREF(entry);
		if (Z_TYPE_P(entry) == IS_LONG) {
			inverse[count] = (double) Z_LVAL_P(entry);
		} else if (Z_TYPE_P(entry) == IS_DOUBLE && zend_finite(Z_DVAL_P(entry))) {
			inverse[count] = Z_DVAL_P(entry);
		} else {
			zend_argument_value_error(4, "must be a list of six finite numbers");
			RETURN_THROWS();
		}
		count++;
	} ZEND_HASH_FOREACH_END();

	if (!phpfb_rect_inside(self, x, y, target_width, target_height)) {
		zend_value_error("The region " ZEND_LONG_FMT "x" ZEND_LONG_FMT " at (" ZEND_LONG_FMT ", " ZEND_LONG_FMT ") is empty or not inside a %dx%d framebuffer.",
			target_width, target_height, x, y, self->width, self->height);
		RETURN_THROWS();
	}
	if (opacity < 0 || opacity > 255) {
		zend_argument_value_error(9, "must be between 0 and 255");
		RETURN_THROWS();
	}
	if (row < 0 || row > FB_MAX_SIDE) {
		zend_argument_value_error(11, "must be between 0 and %d", FB_MAX_SIDE);
		RETURN_THROWS();
	}

	if (tint < 0 || tint > (zend_long) UINT32_MAX) {
		zend_argument_value_error(12, "must be a colour 0xRRGGBBAA");
		RETURN_THROWS();
	}

	double window[4];
	if (window_list != NULL) {
		uint32_t seen = 0;
		if (!zend_array_is_list(window_list) || zend_hash_num_elements(window_list) != 4) {
			zend_argument_value_error(13, "must be a list of four finite numbers [u0, v0, u1, v1]");
			RETURN_THROWS();
		}
		ZEND_HASH_FOREACH_VAL(window_list, entry) {
			ZVAL_DEREF(entry);
			if (Z_TYPE_P(entry) == IS_LONG) {
				window[seen] = (double) Z_LVAL_P(entry);
			} else if (Z_TYPE_P(entry) == IS_DOUBLE && zend_finite(Z_DVAL_P(entry))) {
				window[seen] = Z_DVAL_P(entry);
			} else {
				zend_argument_value_error(13, "must be a list of four finite numbers [u0, v0, u1, v1]");
				RETURN_THROWS();
			}
			seen++;
		} ZEND_HASH_FOREACH_END();
		if (!(window[0] < window[2] && window[1] < window[3])) {
			zend_argument_value_error(13, "must have u0 < u1 and v0 < v1");
			RETURN_THROWS();
		}
	}

	size_t mask_count = 0;
	if (coverage != NULL) {
		if (ZSTR_LEN(coverage) % FB_SPAN_BYTES != 0) {
			zend_argument_value_error(14, "must be a whole number of %d-byte spans, got %zu bytes", FB_SPAN_BYTES, ZSTR_LEN(coverage));
			RETURN_THROWS();
		}
		/* Every span is checked before any pixel is painted: inside the buffer, rows ascending, x ascending without overlap. */
		mask_count = ZSTR_LEN(coverage) / FB_SPAN_BYTES;
		const uint8_t *spans = (const uint8_t *) ZSTR_VAL(coverage);
		int last_y = -1, last_end = 0;
		for (size_t i = 0; i < mask_count; i++) {
			const uint8_t *p = spans + i * FB_SPAN_BYTES;
			int sy = p[0] | (p[1] << 8), sx = p[2] | (p[3] << 8), length = p[4] | (p[5] << 8);

			if (length < 1 || sy >= self->height || length > self->width - sx) {
				zend_argument_value_error(14, "has span %zu empty or not inside a %dx%d framebuffer", i, self->width, self->height);
				RETURN_THROWS();
			}
			if (sy < last_y || (sy == last_y && sx < last_end)) {
				zend_argument_value_error(14, "has span %zu out of order: rows ascend, and x ascends without overlap within a row", i);
				RETURN_THROWS();
			}
			last_y = sy;
			last_end = sx + length;
		}
	}

	fb_paint_rgba8(&self->format, self->bytes, self->width, self->height, (const uint8_t *) ZSTR_VAL(rgba8), (int) width, (int) height,
		inverse, (fb_rect) {(int) x, (int) y, (int) target_width, (int) target_height}, (int) opacity, smooth, (int) row,
		(uint32_t) tint, window_list != NULL ? window : NULL, coverage != NULL ? (const uint8_t *) ZSTR_VAL(coverage) : NULL, mask_count);
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

/* An RGBA8888 buffer, as the glass levels are; a ValueError naming `what` otherwise. */
static bool phpfb_level(const phpfb_buffer *b, const char *what)
{
	if (b->format.layout != FB_LAYOUT_RGBA8888) {
		zend_value_error("%s is not an RGBA8888 buffer.", what);
		return false;
	}

	return true;
}

ZEND_METHOD(FbBuffer, kawaseDown)
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
	if (!phpfb_level(self, "This") || !phpfb_level(source, "The source")) {
		RETURN_THROWS();
	}
	if (x < 0 || y < 0 || width < 1 || height < 1 || width > source->width - x || height > source->height - y) {
		zend_value_error("The region is empty or not inside the %dx%d source.", source->width, source->height);
		RETURN_THROWS();
	}
	if (self->width != (int) ((width + 1) / 2) || self->height != (int) ((height + 1) / 2)) {
		zend_value_error("A level down of " ZEND_LONG_FMT "x" ZEND_LONG_FMT " is " ZEND_LONG_FMT "x" ZEND_LONG_FMT ", this is %dx%d.", width, height, (width + 1) / 2, (height + 1) / 2, self->width, self->height);
		RETURN_THROWS();
	}

	fb_kawase_down(source->bytes, source->width, (fb_rect) {(int) x, (int) y, (int) width, (int) height}, self->bytes, self->width, self->height);
}

ZEND_METHOD(FbBuffer, kawaseUp)
{
	zend_object *source_object;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJ_OF_CLASS(source_object, phpfb_ce_FbBuffer)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	phpfb_buffer *source = phpfb_buffer_from(source_object);
	if (source->bytes == NULL) {
		zend_throw_error(NULL, "FbBuffer has not been constructed");
		RETURN_THROWS();
	}
	if (!phpfb_level(self, "This") || !phpfb_level(source, "The source")) {
		RETURN_THROWS();
	}
	if (self->width > 2 * source->width || self->height > 2 * source->height) {
		zend_value_error("A level up of %dx%d is at most %dx%d, this is %dx%d.", source->width, source->height, 2 * source->width, 2 * source->height, self->width, self->height);
		RETURN_THROWS();
	}

	fb_kawase_up(source->bytes, source->width, source->height, self->bytes, self->width, self->height);
}

/* `count` finite numbers from a list into `out`; a ValueError naming `what` otherwise. */
static bool phpfb_numbers(HashTable *list, int count, double *out, const char *what)
{
	zval *entry;
	int i = 0;

	if (!zend_array_is_list(list) || (int) zend_hash_num_elements(list) != count) {
		zend_value_error("%s is a list of %d numbers.", what, count);
		return false;
	}
	ZEND_HASH_FOREACH_VAL(list, entry) {
		ZVAL_DEREF(entry);
		if (Z_TYPE_P(entry) == IS_LONG) {
			out[i] = (double) Z_LVAL_P(entry);
		} else if (Z_TYPE_P(entry) == IS_DOUBLE && zend_finite(Z_DVAL_P(entry))) {
			out[i] = Z_DVAL_P(entry);
		} else {
			zend_value_error("%s is a list of %d finite numbers.", what, count);
			return false;
		}
		i++;
	} ZEND_HASH_FOREACH_END();

	return true;
}

ZEND_METHOD(FbBuffer, paintGlass)
{
	HashTable *box_list, *radii_list, *look_list;
	zend_object *blur_object, *sharp_object = NULL;
	zend_long scale, origin_x, origin_y, clip_x, clip_y, clip_width, clip_height;

	ZEND_PARSE_PARAMETERS_START(12, 12)
		Z_PARAM_ARRAY_HT(box_list)
		Z_PARAM_ARRAY_HT(radii_list)
		Z_PARAM_OBJ_OF_CLASS(blur_object, phpfb_ce_FbBuffer)
		Z_PARAM_LONG(scale)
		Z_PARAM_OBJ_OF_CLASS_OR_NULL(sharp_object, phpfb_ce_FbBuffer)
		Z_PARAM_LONG(origin_x)
		Z_PARAM_LONG(origin_y)
		Z_PARAM_ARRAY_HT(look_list)
		Z_PARAM_LONG(clip_x)
		Z_PARAM_LONG(clip_y)
		Z_PARAM_LONG(clip_width)
		Z_PARAM_LONG(clip_height)
	ZEND_PARSE_PARAMETERS_END();
	PHPFB_THIS_BUFFER(self);

	fb_glass g;
	double look[9];
	phpfb_buffer *blur = phpfb_buffer_from(blur_object), *sharp = sharp_object != NULL ? phpfb_buffer_from(sharp_object) : NULL;

	if (blur->bytes == NULL || (sharp != NULL && sharp->bytes == NULL)) {
		zend_throw_error(NULL, "FbBuffer has not been constructed");
		RETURN_THROWS();
	}
	if (!phpfb_level(blur, "The blurred level") || (sharp != NULL && !phpfb_level(sharp, "The sharp capture"))
			|| !phpfb_numbers(box_list, 4, g.box, "The box") || !phpfb_numbers(radii_list, 4, g.radii, "The radii")
			|| !phpfb_numbers(look_list, 9, look, "The look")) {
		RETURN_THROWS();
	}
	if (scale < 1 || scale > 1024 || origin_x < -65536 || origin_x > 65536 || origin_y < -65536 || origin_y > 65536) {
		zend_value_error("The scale is 1..1024 and the origin within +/-65536.");
		RETURN_THROWS();
	}
	if (clip_x < 0 || clip_y < 0 || clip_width < 1 || clip_height < 1 || clip_width > self->width - clip_x || clip_height > self->height - clip_y) {
		zend_value_error("The clip is empty or not inside a %dx%d framebuffer.", self->width, self->height);
		RETURN_THROWS();
	}
	for (int k = 0; k < 4; k++) {
		if (fabs(g.box[k]) > 1073741824.0) {
			zend_value_error("The box is past +/-2^30.");
			RETURN_THROWS();
		}
	}
	if (look[6] < 0 || look[6] > 4294967295.0 || look[7] < 0 || look[7] > 4294967295.0 || look[8] < 0 || look[8] > 255) {
		zend_value_error("The look's tint and rim are 0xRRGGBBAA and its alpha 0..255.");
		RETURN_THROWS();
	}

	g.blur = blur->bytes;
	g.blur_width = blur->width;
	g.blur_height = blur->height;
	g.scale = (int) scale;
	g.sharp = sharp != NULL ? sharp->bytes : NULL;
	g.sharp_width = sharp != NULL ? sharp->width : 0;
	g.sharp_height = sharp != NULL ? sharp->height : 0;
	g.origin_x = (int) origin_x;
	g.origin_y = (int) origin_y;
	g.frost = look[0];
	g.refraction = look[1];
	g.bevel = look[2];
	g.saturation = look[3];
	g.light = look[4];
	g.grain = look[5];
	g.tint = (uint32_t) look[6];
	g.rim = (uint32_t) look[7];
	g.alpha = (int) look[8];

	fb_rect bounds;
	if (!fb_paint_glass(&self->format, self->bytes, self->width, self->height, &g, (fb_rect) {(int) clip_x, (int) clip_y, (int) clip_width, (int) clip_height}, &bounds)) {
		RETURN_NULL();
	}
	phpfb_return_rect(return_value, bounds);
}
