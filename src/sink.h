/*
 * The span sink: what another extension (ext-rasterize) calls to paint spans straight into an FbBuffer, without
 * linking to ext-fb. It finds the exported `phpfb_span_sink` through ext-fb's module handle and checks the version.
 * Change the layout only with a new version; ext-rasterize keeps a copy of it in its own src/sink.h.
 */

#ifndef PHPFB_SINK_H
#define PHPFB_SINK_H

#include "php.h"

#define PHPFB_SINK_VERSION 1

typedef struct {
	unsigned version;
	/* An FbBuffer's size: true with *width and *height set, false (nothing thrown) for any other object. */
	bool (*size)(zend_object *buffer, int *width, int *height);
	/*
	 * `count` spans painted at `rgba` as FbBuffer::paintSpans() paints them: 1 with bounds [x, y, width, height]
	 * set, 0 when there were none, -1 with a ValueError thrown and nothing painted.
	 */
	int (*paint_spans)(zend_object *buffer, const uint8_t *spans, size_t count, uint32_t rgba, int bounds[4]);
} phpfb_span_sink_t;

ZEND_DLEXPORT extern const phpfb_span_sink_t phpfb_span_sink;

#endif
