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
#include "php_wordutil.h"

/* If you declare any globals in php_wordutil.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(wordutil)
*/

/* True global resources - no need for thread safety here */
static int le_wordutil;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("wordutil.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_wordutil_globals, wordutil_globals)
    STD_PHP_INI_ENTRY("wordutil.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_wordutil_globals, wordutil_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */

/* {{{ proto string replace_word(string subject)
    */
PHP_FUNCTION(replace_word)
{
	char *subject = NULL;
	int argc = ZEND_NUM_ARGS();
	int subject_len;

	if (zend_parse_parameters(argc TSRMLS_CC, "s", &subject, &subject_len) == FAILURE) 
		return;
	//查找是否包含非法词并替换
	
	//返回替换后的字符串

}
/* }}} */


/* {{{ php_wordutil_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_wordutil_init_globals(zend_wordutil_globals *wordutil_globals)
{
	wordutil_globals->global_value = 0;
	wordutil_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(wordutil)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(wordutil)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(wordutil)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(wordutil)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(wordutil)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "wordutil support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ wordutil_functions[]
 *
 * Every user visible function must have an entry in wordutil_functions[].
 */
const zend_function_entry wordutil_functions[] = {
	PHP_FE(confirm_wordutil_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE(replace_word,	NULL)
	PHP_FE_END	/* Must be the last line in wordutil_functions[] */
};
/* }}} */

/* {{{ wordutil_module_entry
 */
zend_module_entry wordutil_module_entry = {
	STANDARD_MODULE_HEADER,
	"wordutil",
	wordutil_functions,
	PHP_MINIT(wordutil),
	PHP_MSHUTDOWN(wordutil),
	PHP_RINIT(wordutil),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(wordutil),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(wordutil),
	PHP_WORDUTIL_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_WORDUTIL
ZEND_GET_MODULE(wordutil)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
