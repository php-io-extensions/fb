
/* This file was generated automatically by Zephir do not modify it! */

#ifndef PHP_FB_H
#define PHP_FB_H 1

#ifdef PHP_WIN32
#define ZEPHIR_RELEASE 1
#endif

#include "kernel/globals.h"

#define PHP_FB_NAME        "fb"
#define PHP_FB_VERSION     "0.8.0"
#define PHP_FB_EXTNAME     "fb"
#define PHP_FB_AUTHOR      "Project Saturn Studios, LLC"
#define PHP_FB_ZEPVERSION  "0.19.0-$Id$"
#define PHP_FB_DESCRIPTION "Framebuffers for the Venusian Surface CPU renderers: five buffer kinds in C"



ZEND_BEGIN_MODULE_GLOBALS(fb)

	int initialized;

	/** Function cache */
	HashTable *fcache;

	zephir_fcall_cache_entry *scache[ZEPHIR_MAX_CACHE_SLOTS];

	/* Cache enabled */
	unsigned int cache_enabled;

	/* Max recursion control */
	unsigned int recursive_lock;

	
ZEND_END_MODULE_GLOBALS(fb)

#ifdef ZTS
#include "TSRM.h"
#endif

ZEND_EXTERN_MODULE_GLOBALS(fb)

#ifdef ZTS
	#define ZEPHIR_GLOBAL(v) ZEND_MODULE_GLOBALS_ACCESSOR(fb, v)
#else
	#define ZEPHIR_GLOBAL(v) (fb_globals.v)
#endif

#ifdef ZTS
	ZEND_TSRMLS_CACHE_EXTERN()
	#define ZEPHIR_VGLOBAL ((zend_fb_globals *) (*((void ***) tsrm_get_ls_cache()))[TSRM_UNSHUFFLE_RSRC_ID(fb_globals_id)])
#else
	#define ZEPHIR_VGLOBAL &(fb_globals)
#endif

#define ZEPHIR_API ZEND_API

#define zephir_globals_def fb_globals
#define zend_zephir_globals_def zend_fb_globals

extern zend_module_entry fb_module_entry;
#define phpext_fb_ptr &fb_module_entry

#endif
