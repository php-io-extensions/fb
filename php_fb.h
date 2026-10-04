#ifndef PHP_FB_H
#define PHP_FB_H

extern zend_module_entry fb_module_entry;
#define phpext_fb_ptr &fb_module_entry

#define PHP_FB_VERSION "0.10.0"

#if defined(ZTS) && defined(COMPILE_DL_FB)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif
