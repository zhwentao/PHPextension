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

#include <sys/time.h>
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_sg_monitor.h"
#include "shmcache.h"
#include "monitor_type.h"

#include "SAPI.h"

/* Utils for PHP 7 */
#if PHP_VERSION_ID < 70000
#define P7_EX_OBJ(ex)   ex->object
#define P7_EX_OBJCE(ex) Z_OBJCE_P(ex->object)
#define P7_EX_OPARR(ex) ex->op_array
#define P7_STR(v)       v
#define P7_STR_LEN(v)   strlen(v)
#else
#define P7_EX_OBJ(ex)   Z_OBJ(ex->This)
#define P7_EX_OBJCE(ex) Z_OBJCE(ex->This)
#define P7_EX_OPARR(ex) (&(ex->func->op_array))
#define P7_STR(v)       ZSTR_VAL(v)
#define P7_STR_LEN(v)   ZSTR_LEN(v)
#endif

/*
 * Function declarations
 */
static int uri_monitor_init();
static int uri_send_stat();
static int uri_monitor_filter();
static inline zend_function *obtain_zend_function(zend_bool internal, zend_execute_data *ex, zend_op_array *op_array TSRMLS_DC);
static long filter_frame(zend_bool internal, zend_execute_data *ex, zend_op_array *op_array TSRMLS_DC);
#if PHP_VERSION_ID < 50500
static void (*ori_execute)(zend_op_array *op_array TSRMLS_DC);
static void (*ori_execute_internal)(zend_execute_data *execute_data_ptr, int return_value_used TSRMLS_DC);
ZEND_API void pm_execute(zend_op_array *op_array TSRMLS_DC);
ZEND_API void pm_execute_internal(zend_execute_data *execute_data, int return_value_used TSRMLS_DC);
#elif PHP_VERSION_ID < 70000
static void (*ori_execute_ex)(zend_execute_data *execute_data TSRMLS_DC);
static void (*ori_execute_internal)(zend_execute_data *execute_data_ptr, zend_fcall_info *fci, int return_value_used TSRMLS_DC);
ZEND_API void pm_execute_ex(zend_execute_data *execute_data TSRMLS_DC);
ZEND_API void pm_execute_internal(zend_execute_data *execute_data, zend_fcall_info *fci, int return_value_used TSRMLS_DC);
#else
static void (*ori_execute_ex)(zend_execute_data *execute_data);
static void (*ori_execute_internal)(zend_execute_data *execute_data, zval *return_value);
ZEND_API void pm_execute_ex(zend_execute_data *execute_data);
ZEND_API void pm_execute_internal(zend_execute_data *execute_data, zval *return_value);
#endif

/* globals in php_sg_monitor.h
 */
ZEND_DECLARE_MODULE_GLOBALS(sg_monitor)

/* True global resources - no need for thread safety here */
static int le_sg_monitor;

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("sg_monitor.enable",      "1", PHP_INI_SYSTEM, OnUpdateLong, enable, zend_sg_monitor_globals, sg_monitor_globals)
    STD_PHP_INI_ENTRY("sg_monitor.domains", "", PHP_INI_SYSTEM, OnUpdateString, domains, zend_sg_monitor_globals, sg_monitor_globals)
    STD_PHP_INI_ENTRY("sg_monitor.function_names", "", PHP_INI_SYSTEM, OnUpdateString, function_names, zend_sg_monitor_globals, sg_monitor_globals)
    STD_PHP_INI_ENTRY("sg_monitor.shmcache_conf", "", PHP_INI_SYSTEM, OnUpdateString, shmcache_conf, zend_sg_monitor_globals, sg_monitor_globals)
PHP_INI_END()
/* }}} */


/* {{{ php_sg_monitor_init_globals
 */
static void php_sg_monitor_init_globals(zend_sg_monitor_globals *sg_monitor_globals)
{
	sg_monitor_globals->enable = 0;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(sg_monitor)
{
	REGISTER_INI_ENTRIES();
    /* Replace executor */
#if PHP_VERSION_ID < 50500
    ori_execute = zend_execute;
    zend_execute = pm_execute;
#else
    ori_execute_ex = zend_execute_ex;
    zend_execute_ex = pm_execute_ex;
#endif
    ori_execute_internal = zend_execute_internal;
    zend_execute_internal = pm_execute_internal;

	//init shmcache
	if ((shm_result = shmcache_init_from_file(&SMG(monitor_context), SMG(shmcache_conf))) != 0) {
        SMG(enable) = 0;
	}
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(sg_monitor)
{
	UNREGISTER_INI_ENTRIES();
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
	if (uri_monitor_filter()) {
	    uri_monitor_init();
	}
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(sg_monitor)
{
	if (SMG(uri_need_monitor)) {
	    uri_send_stat();
	}
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

	DISPLAY_INI_ENTRIES();
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


/**
 * Obtain zend function
 * -------------------
 */
static inline zend_function *obtain_zend_function(zend_bool internal, zend_execute_data *ex, zend_op_array *op_array TSRMLS_DC)
{
#if PHP_VERSION_ID < 50500
    if (internal || ex) {
        return ex->function_state.function;
    } else {
        return (zend_function *) op_array;
    }
#elif PHP_VERSION_ID < 70000
    return ex->function_state.function;
#else
    return ex->func;
#endif
}


/* {{{ filter_frame 
 * Filter frame by functin name
 */
static long filter_frame(zend_bool internal, zend_execute_data *ex, zend_op_array *op_array TSRMLS_DC)
{
    long dotrace = 0; 

    zend_function *zf = obtain_zend_function(internal, ex, op_array);
    
    /* Filter function */
    if((zf->common.function_name) && 
			P7_STR_LEN(SMG(function_names)) &&
			strstr(P7_STR(zf->common.function_name), SMG(function_names)) != NULL) {
        dotrace = SMG(enable);
    }
    return dotrace;
}
/* }}} */

/* {{{ uri_monitor
 * monitor URI, record uri start time, create shmcache frame
 */
static int uri_monitor_init() 
{
    //record uri, cpu, mem, start_time to uri_stat struct
	SMG(uri_str) = SG(request_info).request_uri;
	gettimeofday(&SMG(uri_start_time), NULL);
	SMG(sg_uri_stat).uri_count += 1;
}
/* }}} */

/* {{{ uri_monitor_filter
 * filter dommain 
 * set global var uri_need_monitor 1 need monitor, 0 skip monitor
 */
static int uri_monitor_filter()
{
	//TODO filter domain
	SMG(uri_need_monitor) = (1 && SMG(enable));
	return SMG(uri_need_monitor);
}
/* }}} */

/* {{{ uri_send_stat
 * send uri statistic to share memory
 * TODO shm init/set, cpu/mem usage, struct serialize
 */
static int uri_send_stat()
{
    //record uri, cpu, mem, spend_time
	int shm_result;
    int index;
    char *config_filename;
    struct shmcache_key_info key;
    char *value;
    int value_len;
    int ttl;
	

	//set shmcache
    key.data = argv[index++];
    key.length = strlen(key.data);
    value = argv[index++];
    value_len = strlen(value);
    ttl = atoi(argv[index++]);
	shm_result = shmcache_set(&SMG(monitor_context), &key, value, value_len, ttl);

	//error log
	if (shm_result != 0) {
	
	}
}
/* }}} */

/* {{{ pm_execute_core
 * Trace Executor Replacement
 * --------------------
 */ 
#if PHP_VERSION_ID < 50500
ZEND_API void pm_execute_core(int internal, zend_execute_data *execute_data, zend_op_array *op_array, int rvu TSRMLS_DC)
#elif PHP_VERSION_ID < 70000
ZEND_API void pm_execute_core(int internal, zend_execute_data *execute_data, zend_fcall_info *fci, int rvu TSRMLS_DC)
#else
ZEND_API void pm_execute_core(int internal, zend_execute_data *execute_data, zval *return_value)
#endif
{
    long domonitor;
	zend_bool dobailout = 0;

	/*
	 * TODO list:  
	 * normal scenarios, exception/failed/error scenarios
	 * monitor filter
	 * save frame info
	 */

    /* Filter frame by function name*/
#if PHP_VERSION_ID < 50500
    domonitor = filter_frame(internal, execute_data, op_array TSRMLS_CC);
#else
    domonitor = filter_frame(internal, execute_data, NULL TSRMLS_CC);
#endif
  
	if (domonitor) {
	
	}

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

	if (domonitor) {
	
	}

	if (dobailout) {
	    zend_bailout();
	}
}
/* }}} */


/* {{{ pm_execute pm_execute_internal
 * 
 */
#if PHP_VERSION_ID < 50500
ZEND_API void pm_execute(zend_op_array *op_array TSRMLS_DC)
{
    pm_execute_core(0, EG(current_execute_data), op_array, 0 TSRMLS_CC);
}

ZEND_API void pm_execute_internal(zend_execute_data *execute_data, int return_value_used TSRMLS_DC)
{
    pm_execute_core(1, execute_data, NULL, return_value_used TSRMLS_CC);
}
#elif PHP_VERSION_ID < 70000
ZEND_API void pm_execute_ex(zend_execute_data *execute_data TSRMLS_DC)
{
    pm_execute_core(0, execute_data, NULL, 0 TSRMLS_CC);
}

ZEND_API void pm_execute_internal(zend_execute_data *execute_data, zend_fcall_info *fci, int return_value_used TSRMLS_DC)
{
    pm_execute_core(1, execute_data, fci, return_value_used TSRMLS_CC);
}
#else
ZEND_API void pm_execute_ex(zend_execute_data *execute_data)
{
    pm_execute_core(0, execute_data, NULL);
}

ZEND_API void pm_execute_internal(zend_execute_data *execute_data, zval *return_value)
{
    pm_execute_core(1, execute_data, return_value);
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
