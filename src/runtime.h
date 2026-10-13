/*
 * What the class sources share: the Zend objects that hold a format and a
 * buffer, and their class entries.
 */

#ifndef PHPFB_RUNTIME_H
#define PHPFB_RUNTIME_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "php_fb.h"
#include "core.h"

typedef struct {
	fb_format format;
	bool resolved;             /* false until __construct has run */
	zend_object std;
} phpfb_format;

typedef struct {
	fb_format format;
	zval format_object;        /* the FbFormat this buffer was made with */
	int width;
	int height;
	size_t size;
	uint8_t *bytes;            /* NULL until __construct has run */
	zend_object std;
} phpfb_buffer;

static zend_always_inline phpfb_format *phpfb_format_from(zend_object *obj)
{
	return (phpfb_format *) ((char *) obj - XtOffsetOf(phpfb_format, std));
}

static zend_always_inline phpfb_buffer *phpfb_buffer_from(zend_object *obj)
{
	return (phpfb_buffer *) ((char *) obj - XtOffsetOf(phpfb_buffer, std));
}

extern zend_class_entry *phpfb_ce_FbFormat;
extern zend_class_entry *phpfb_ce_FbBuffer;

void phpfb_register_FbFormat(void);
void phpfb_register_FbBuffer(void);

/* A zend_long as an int; -1 when it does not fit, which no argument here accepts. */
static zend_always_inline int phpfb_narrow(zend_long value)
{
	return value < 0 || value > INT_MAX ? -1 : (int) value;
}

/*
 * Whether a width x height surface can be addressed: (width + 8) x height x 4
 * bounds the bytes of every layout (planar's sixteen padded planes included)
 * and of its RGBA8 form, and must fit a zend_long, which also bounds size_t.
 * Always true on 64-bit; on 32-bit it refuses sides whose product would wrap
 * an allocation.
 */
static zend_always_inline bool phpfb_addressable(zend_long width, zend_long height)
{
	return ((uint64_t) width + 8u) * (uint64_t) height * 4u <= (uint64_t) ZEND_LONG_MAX;
}

#endif
