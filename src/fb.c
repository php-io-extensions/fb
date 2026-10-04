/*
 * fb: pixel storage in C. FbFormat describes how pixels are stored; FbBuffer
 * holds one surface in that format and reads, writes, converts and copies it.
 */

#include "runtime.h"
#include "ext/standard/info.h"
#include "../stubs/fb_arginfo.h"

PHP_MINIT_FUNCTION(fb)
{
	(void) type;

	register_fb_symbols(module_number);

	phpfb_register_FbFormat();
	phpfb_register_FbBuffer();

	return SUCCESS;
}

PHP_RINIT_FUNCTION(fb)
{
	(void) type;
	(void) module_number;
#if defined(COMPILE_DL_FB) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}

PHP_MINFO_FUNCTION(fb)
{
	(void) zend_module;

	php_info_print_table_start();
	php_info_print_table_row(2, "fb support", "enabled");
	php_info_print_table_row(2, "Version", PHP_FB_VERSION);
	php_info_print_table_end();
}

zend_module_entry fb_module_entry = {
	STANDARD_MODULE_HEADER,
	"fb",
	NULL,
	PHP_MINIT(fb),
	NULL,
	PHP_RINIT(fb),
	NULL,
	PHP_MINFO(fb),
	PHP_FB_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_FB
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(fb)
#endif
