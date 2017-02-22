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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_sg_monitor.h"

#if PHP_VERSION_ID < 50500
static void (*ori_execute)(zend_op_array *op_array TSRMLS_DC);
static void (*ori_execute_internal)(zend_execute_data *execute_data_ptr, int return_value_used TSRMLS_DC);
ZEND_API void pt_execute(zend_op_array *op_array TSRMLS_DC);
ZEND_API void pt_execute_internal(zend_execute_data *execute_data, int return_value_used TSRMLS_DC);
#elif PHP_VERSION_ID < 70000
static void (*ori_execute_ex)(zend_execute_data *execute_data TSRMLS_DC);
static void (*ori_execute_internal)(zend_execute_data *execute_data_ptr, zend_fcall_info *fci, int return_value_used TSRMLS_DC);
ZEND_API void pt_execute_ex(zend_execute_data *execute_data TSRMLS_DC);
ZEND_API void pt_execute_internal(zend_execute_data *execute_data, zend_fcall_info *fci, int return_value_used TSRMLS_DC);
#else
static void (*ori_execute_ex)(zend_execute_data *execute_data);
static void (*ori_execute_internal)(zend_execute_data *execute_data, zval *return_value);
ZEND_API void pt_execute_ex(zend_execute_data *execute_data);
ZEND_API void pt_execute_internal(zend_execute_data *execute_data, zval *return_value);
#endif

/* If you declare any globals in php_sg_monitor.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(sg_monitor)
*/

/* True global resources - no need for thread safety here */
static int le_sg_monitor;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("sg_monitor.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_sg_monitor_globals, sg_monitor_globals)
    STD_PHP_INI_ENTRY("sg_monitor.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_sg_monitor_globals, sg_monitor_globals)
PHP_INI_END()
*/
/* }}} */


/* {{{ php_sg_monitor_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_sg_monitor_init_globals(zend_sg_monitor_globals *sg_monitor_globals)
{
	sg_monitor_globals->global_value = 0;
	sg_monitor_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(sg_monitor)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
    /* Replace executor */
#if PHP_VERSION_ID < 50500
    ori_execute = zend_execute;
    zend_execute = pt_execute;
#else
    ori_execute_ex = zend_execute_ex;
    zend_execute_ex = pt_execute_ex;
#endif
    ori_execute_internal = zend_execute_internal;
    zend_execute_internal = pt_execute_internal;
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(sg_monitor)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
    /* Restore original executor */
#if PHP_VERSION_ID < 50500
    zend_execute = ori_execute;
#else
    zend_execute_ex = ori_execute_ex;
#endif
    zend_execute_internal = ori_execute_internal;
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(sg_monitor)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(sg_monitor)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(sg_monitor)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "sg_monitor support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ sg_monitor_functions[]
 *
 * Every user visible function must have an entry in sg_monitor_functions[].
 */
const zend_function_entry sg_monitor_functions[] = {
	PHP_FE_END	/* Must be the last line in sg_monitor_functions[] */
};
/* }}} */

/* {{{ sg_monitor_module_entry
 */
zend_module_entry sg_monitor_module_entry = {
	STANDARD_MODULE_HEADER,
	"sg_monitor",
	sg_monitor_functions,
	PHP_MINIT(sg_monitor),
	PHP_MSHUTDOWN(sg_monitor),
	PHP_RINIT(sg_monitor),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(sg_monitor),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(sg_monitor),
	PHP_SG_MONITOR_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

/* {{{ pt_execute_core
 * Trace Executor Replacement
 * --------------------
 */ 
#if PHP_VERSION_ID < 50500
ZEND_API void pt_execute_core(int internal, zend_execute_data *execute_data, zend_op_array *op_array, int rvu TSRMLS_DC)
#elif PHP_VERSION_ID < 70000
ZEND_API void pt_execute_core(int internal, zend_execute_data *execute_data, zend_fcall_info *fci, int rvu TSRMLS_DC)
#else
ZEND_API void pt_execute_core(int internal, zend_execute_data *execute_data, zval *return_value)
#endif
{

	/*
	 * TODO list:  
	 * normal scenarios, exception/failed/error scenarios
	 * monitor filter
	 * save frame info
	 */

    zend_try {
#if PHP_VERSION_ID < 50500
        if (internal) {
            if (ori_execute_internal) {
                ori_execute_internal(execute_data, rvu TSRMLS_CC);
            } else {
                execute_internal(execute_data, rvu TSRMLS_CC);
            }
        } else {
            ori_execute(op_array TSRMLS_CC);
        }
#elif PHP_VERSION_ID < 70000
        if (internal) {
            if (ori_execute_internal) {
                ori_execute_internal(execute_data, fci, rvu TSRMLS_CC);
            } else {
                execute_internal(execute_data, fci, rvu TSRMLS_CC);
            }
        } else {
            ori_execute_ex(execute_data TSRMLS_CC);
        }
#else
        if (internal) {
            if (ori_execute_internal) {
                ori_execute_internal(execute_data, return_value);
            } else {
                execute_internal(execute_data, return_value);
            }
        } else {
            ori_execute_ex(execute_data);
        }
#endif
    } zend_catch {
	
	} zend_end_try();
}
/* }}} */


/* {{{ pt_execute pt_execute_internal
 * 
 */
#if PHP_VERSION_ID < 50500
ZEND_API void pt_execute(zend_op_array *op_array TSRMLS_DC)
{
    pt_execute_core(0, EG(current_execute_data), op_array, 0 TSRMLS_CC);
}

ZEND_API void pt_execute_internal(zend_execute_data *execute_data, int return_value_used TSRMLS_DC)
{
    pt_execute_core(1, execute_data, NULL, return_value_used TSRMLS_CC);
}
#elif PHP_VERSION_ID < 70000
ZEND_API void pt_execute_ex(zend_execute_data *execute_data TSRMLS_DC)
{
    pt_execute_core(0, execute_data, NULL, 0 TSRMLS_CC);
}

ZEND_API void pt_execute_internal(zend_execute_data *execute_data, zend_fcall_info *fci, int return_value_used TSRMLS_DC)
{
    pt_execute_core(1, execute_data, fci, return_value_used TSRMLS_CC);
}
#else
ZEND_API void pt_execute_ex(zend_execute_data *execute_data)
{
    pt_execute_core(0, execute_data, NULL);
}

ZEND_API void pt_execute_internal(zend_execute_data *execute_data, zval *return_value)
{
    pt_execute_core(1, execute_data, return_value);
}
#endif
/* }}} */

#ifdef COMPILE_DL_SG_MONITOR
ZEND_GET_MODULE(sg_monitor)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
