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
#include "ahocorasick/ahocorasick.h"

#define PATTERN(p,r)    {{p,sizeof(p)-1},{r,sizeof(r)-1},{{0},0}}
#define CHUNK(c)        {c,sizeof(c)-1}

AC_PATTERN_t patterns[] = {
    PATTERN("city", "[S1]"),    /* Replace "simplicity" with "[S1]" */
    PATTERN("the ", ""),        /* Replace "the " with an empty string */
    PATTERN("and", NULL),       /* Do not replace "and" */
    PATTERN("experience", "[S2]"),
    PATTERN("exp", "[S3]"),
    PATTERN("simplicity", "[S4]"),
    PATTERN("ease", "[S5]"),
};
#define PATTERN_COUNT (sizeof(patterns)/sizeof(AC_PATTERN_t))

AC_ALPHABET_t *chunk1 = "北#京天1安门aaa the ease and simplicity of multifast";

static void listener (AC_TEXT_t *text, void *user);

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

    unsigned int i;
    AC_TRIE_t *trie;
    AC_TEXT_t chunk;

	if (zend_parse_parameters(argc TSRMLS_CC, "s", &subject, &subject_len) == FAILURE) 
		return;
	//查找是否包含非法词并替换
	trie = ac_trie_create();
	for (i=0; i < PATTERN_COUNT; i++) {
		if (ac_trie_add(trie, &patterns[i], 0) != ACERR_SUCCESS) {
			printf("Failed to add pattern");
		}
	}
	ac_trie_finalize(trie);
	chunk.astring = chunk1;
	chunk.length = strlen(chunk.astring);

    multifast_replace (trie, &chunk, MF_REPLACE_MODE_NORMAL, listener, 0);
    multifast_rep_flush (trie, 0);

    //ac_trie_release (trie);
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

/* {{{ gen_ac_trie_from_file
 */
static void gen_ac_trie_from_file()
{
	//获取ini模式文件路径配置项
	//读取文件内容，生成ac_trie
}
/* }}} */

void listener (AC_TEXT_t *text, void *user)
{
    printf ("%.*s", (int)text->length, text->astring);
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(wordutil)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	// todo 加载非法词字典树
	// 保存在全局变量
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
	//todo 检测字典树文件是否更新，重新加载字典树
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
