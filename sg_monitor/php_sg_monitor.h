/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_SG_MONITOR_H
#define PHP_SG_MONITOR_H

extern zend_module_entry sg_monitor_module_entry;
#define phpext_sg_monitor_ptr &sg_monitor_module_entry

#define PHP_SG_MONITOR_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_SG_MONITOR_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_SG_MONITOR_API __attribute__ ((visibility("default")))
#else
#	define PHP_SG_MONITOR_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include <sys/time.h>
#include <msgpack.h>
#include "monitor_type.h"
#include "shmcache.h"
#include "ext/standard/info.h"
#include "ext/standard/md5.h"
#include "ext/standard/php_smart_str.h" /* for smart_str */
/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     
*/

ZEND_BEGIN_MODULE_GLOBALS(sg_monitor)
	/**
	 *config var
	 */
	zend_bool enable;
	char *domains;
	char *function_names;
	char *shmcache_conf;

	/**
	 * Monitor ctrl var
	 */
	zend_bool uri_need_monitor;
	struct shmcache_context monitor_context;

	/**
	 * Monitor var
	 */
	char *uri_str;
	struct timeval uri_start_time;
    uri_stat sg_uri_stat;

ZEND_END_MODULE_GLOBALS(sg_monitor)

/* In every utility function you add that needs to use variables 
   in php_sg_monitor_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as SMG(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define SMG(v) TSRMG(sg_monitor_globals_id, zend_sg_monitor_globals *, v)
#else
#define SMG(v) (sg_monitor_globals.v)
#endif

#endif	/* PHP_SG_MONITOR_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
