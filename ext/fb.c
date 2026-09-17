
/* This file was generated automatically by Zephir do not modify it! */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>

#include "php_ext.h"
#include "fb.h"

#include <ext/standard/info.h>

#include <Zend/zend_operators.h>
#include <Zend/zend_exceptions.h>
#include <Zend/zend_interfaces.h>

#include "kernel/globals.h"
#include "kernel/main.h"
#include "kernel/fcall.h"
#include "kernel/memory.h"



zend_class_entry *fb_buffer_ce;

ZEND_DECLARE_MODULE_GLOBALS(fb)

PHP_INI_BEGIN()
	
PHP_INI_END()

static PHP_MINIT_FUNCTION(fb)
{
	REGISTER_INI_ENTRIES();
	zephir_module_init();
	ZEPHIR_INIT(Fb_Buffer);
	
	return SUCCESS;
}

#ifndef ZEPHIR_RELEASE
static PHP_MSHUTDOWN_FUNCTION(fb)
{
	
	zephir_deinitialize_memory();
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
#endif

/**
 * Initialize globals on each request or each thread started
 */
static void php_zephir_init_globals(zend_fb_globals *fb_globals)
{
	fb_globals->initialized = 0;

	/* Cache Enabled */
	fb_globals->cache_enabled = 1;

	/* Recursive Lock */
	fb_globals->recursive_lock = 0;

	/* Static cache */
	memset(fb_globals->scache, '\0', sizeof(zephir_fcall_cache_entry*) * ZEPHIR_MAX_CACHE_SLOTS);

	
	
}

/**
 * Initialize globals only on each thread started
 */
static void php_zephir_init_module_globals(zend_fb_globals *fb_globals)
{
	
}

static PHP_RINIT_FUNCTION(fb)
{
	zend_fb_globals *fb_globals_ptr;
	fb_globals_ptr = ZEPHIR_VGLOBAL;

	php_zephir_init_globals(fb_globals_ptr);
	zephir_initialize_memory(fb_globals_ptr);

	
	return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(fb)
{
	
	zephir_deinitialize_memory();
	return SUCCESS;
}



static PHP_MINFO_FUNCTION(fb)
{
	php_info_print_box_start(0);
	php_printf("%s", PHP_FB_DESCRIPTION);
	php_info_print_box_end();

	php_info_print_table_start();
	php_info_print_table_header(2, PHP_FB_NAME, "enabled");
	php_info_print_table_row(2, "Author", PHP_FB_AUTHOR);
	php_info_print_table_row(2, "Version", PHP_FB_VERSION);
	php_info_print_table_row(2, "Build Date", __DATE__ " " __TIME__ );
	php_info_print_table_row(2, "Powered by Zephir", "Version " PHP_FB_ZEPVERSION);
	php_info_print_table_end();
	
	DISPLAY_INI_ENTRIES();
}

static PHP_GINIT_FUNCTION(fb)
{
#if defined(COMPILE_DL_FB) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	php_zephir_init_globals(fb_globals);
	php_zephir_init_module_globals(fb_globals);
}

static PHP_GSHUTDOWN_FUNCTION(fb)
{
	
}


zend_function_entry php_fb_functions[] = {
	ZEND_FE_END

};

static const zend_module_dep php_fb_deps[] = {
	
	ZEND_MOD_END
};

zend_module_entry fb_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	php_fb_deps,
	PHP_FB_EXTNAME,
	php_fb_functions,
	PHP_MINIT(fb),
#ifndef ZEPHIR_RELEASE
	PHP_MSHUTDOWN(fb),
#else
	NULL,
#endif
	PHP_RINIT(fb),
	PHP_RSHUTDOWN(fb),
	PHP_MINFO(fb),
	PHP_FB_VERSION,
	ZEND_MODULE_GLOBALS(fb),
	PHP_GINIT(fb),
	PHP_GSHUTDOWN(fb),
#ifdef ZEPHIR_POST_REQUEST
	PHP_PRSHUTDOWN(fb),
#else
	NULL,
#endif
	STANDARD_MODULE_PROPERTIES_EX
};

/* implement standard "stub" routine to introduce ourselves to Zend */
#ifdef COMPILE_DL_FB
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(fb)
#endif
