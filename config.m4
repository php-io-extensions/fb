PHP_ARG_ENABLE([fb],
  [whether to enable fb support],
  [AS_HELP_STRING([--enable-fb], [Enable the fb pixel-store extension])],
  [no])

if test "$PHP_FB" != "no"; then
  dnl -ffp-contract=off: no fused multiply-add, so paintRgba8() maps every pixel centre the same way on every platform.
  PHP_NEW_EXTENSION([fb],
    [src/fb.c src/core.c src/FbFormat.c src/FbBuffer.c],
    [$ext_shared],, [-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 -ffp-contract=off])
  PHP_ADD_BUILD_DIR([$ext_builddir/src])
fi
