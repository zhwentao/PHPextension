dnl $Id$
dnl config.m4 for extension sg_monitor

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(sg_monitor, for sg_monitor support,
dnl Make sure that the comment is aligned:
dnl [  --with-sg_monitor             Include sg_monitor support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(sg_monitor, whether to enable sg_monitor support,
dnl Make sure that the comment is aligned:
[  --enable-sg_monitor           Enable sg_monitor support])

if test "$PHP_SG_MONITOR" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-sg_monitor -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/sg_monitor.h"  # you most likely want to change this
  dnl if test -r $PHP_SG_MONITOR/$SEARCH_FOR; then # path given as parameter
  dnl   SG_MONITOR_DIR=$PHP_SG_MONITOR
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for sg_monitor files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       SG_MONITOR_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$SG_MONITOR_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the sg_monitor distribution])
  dnl fi


  if test -z "$ROOT"; then
	ROOT=/usr
  fi
  dnl # --with-sg_monitor -> add include path
  dnl PHP_ADD_INCLUDE($SG_MONITOR_DIR/include)

  PHP_ADD_INCLUDE($ROOT/include/fastcommon)
  PHP_ADD_INCLUDE($ROOT/include/shmcache)
 
  PHP_ADD_LIBRARY_WITH_PATH(fastcommon, $ROOT/lib, SG_MONITOR_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(shmcache, $ROOT/lib, SG_MONITOR_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(msgpackc, /usr/local/lib, SG_MONITOR_SHARED_LIBADD)

  dnl # --with-sg_monitor -> check for lib and symbol presence
  dnl LIBNAME=sg_monitor # you may want to change this
  dnl LIBSYMBOL=sg_monitor # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $SG_MONITOR_DIR/$PHP_LIBDIR, SG_MONITOR_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_SG_MONITORLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong sg_monitor lib version or lib not found])
  dnl ],[
  dnl   -L$SG_MONITOR_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  PHP_SUBST(SG_MONITOR_SHARED_LIBADD)

  PHP_NEW_EXTENSION(sg_monitor, sg_monitor.c, $ext_shared)
fi
