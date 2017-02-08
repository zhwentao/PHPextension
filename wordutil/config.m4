dnl $Id$
dnl config.m4 for extension wordutil

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(wordutil, for wordutil support,
dnl Make sure that the comment is aligned:
dnl [  --with-wordutil             Include wordutil support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(wordutil, whether to enable wordutil support,
dnl Make sure that the comment is aligned:
[  --enable-wordutil           Enable wordutil support])

if test "$PHP_WORDUTIL" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-wordutil -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/wordutil.h"  # you most likely want to change this
  dnl if test -r $PHP_WORDUTIL/$SEARCH_FOR; then # path given as parameter
  dnl   WORDUTIL_DIR=$PHP_WORDUTIL
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for wordutil files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       WORDUTIL_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$WORDUTIL_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the wordutil distribution])
  dnl fi

  dnl # --with-wordutil -> add include path
  dnl PHP_ADD_INCLUDE($WORDUTIL_DIR/include)

  dnl # --with-wordutil -> check for lib and symbol presence
  dnl LIBNAME=wordutil # you may want to change this
  dnl LIBSYMBOL=wordutil # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $WORDUTIL_DIR/$PHP_LIBDIR, WORDUTIL_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_WORDUTILLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong wordutil lib version or lib not found])
  dnl ],[
  dnl   -L$WORDUTIL_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(WORDUTIL_SHARED_LIBADD)

  PHP_NEW_EXTENSION(wordutil, wordutil.c ahocorasick/node.c ahocorasick/ahocorasick.c ahocorasick/mpool.c ahocorasick/replace.c, $ext_shared)
fi
