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

#include <sys/stat.h>
#include <unistd.h>
#include <wchar.h>

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_wordutil.h"

#define PATTERN(p,r)    {{p,sizeof(p)-1},{r,sizeof(r)-1},{{0},0}}
#define CHUNK(c)        {c,sizeof(c)-1}

AC_PATTERN_t patterns[] = {
    PATTERN("city", "[S1]"),    /* Replace "simplicity" with "[S1]" */
    PATTERN("the ", ""),        /* Replace "the " with an empty string */
    PATTERN("滴滴", ""),        /* Replace "滴滴" with an empty string */
    PATTERN("阿里", ""),        /* Replace "阿里" with an empty string */
    PATTERN("and", NULL),       /* Do not replace "and" */
    PATTERN("experience", "[S2]"),
    PATTERN("exp", "[S3]"),
    PATTERN("simplicity", "[S4]"),
    PATTERN("ease", "[S5]"),
};
#define PATTERN_COUNT (sizeof(patterns)/sizeof(AC_PATTERN_t))

static void listener (AC_TEXT_t *text, void *user);
static void init_trie_by_var();
static int add_trie_by_file(char* filename, long filesize);
/*
 * 字典树初始化
 */
static int init_ac_trie(char *filemane, long filesize);

/* If you declare any globals in php_wordutil.h uncomment this:
*/
ZEND_DECLARE_MODULE_GLOBALS(wordutil)

/* True global resources - no need for thread safety here */
static int le_wordutil;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
*/
PHP_INI_BEGIN()
	/*
	 * 模式配置文件的绝对路径
	 */
    STD_PHP_INI_ENTRY("wordutil.patterns_path",      "patterns", PHP_INI_SYSTEM, OnUpdateString, patterns_path, zend_wordutil_globals, wordutil_globals)
PHP_INI_END()
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
    AC_TEXT_t chunk;

	if (zend_parse_parameters(argc TSRMLS_CC, "s", &subject, &subject_len) == FAILURE) 
		return;
	chunk.astring = subject;
	chunk.length = strlen(chunk.astring);

    multifast_replace (WORDUTIL_G(trie), &chunk, MF_REPLACE_MODE_NORMAL, listener, 0);
    multifast_rep_flush (WORDUTIL_G(trie), 0);

	//返回替换后的字符串
    RETURN_STRING(WORDUTIL_G(trie)->repdata.buffer.astring, strlen(WORDUTIL_G(trie)->repdata.buffer.astring));
}
/* }}} */


/* {{{ php_wordutil_init_globals
 */
/* Uncomment this function if you have INI entries
*/
static void php_wordutil_init_globals(zend_wordutil_globals *wordutil_globals)
{
	wordutil_globals->patterns_path = 0;
	wordutil_globals->trie = NULL;
	wordutil_globals->pattern_conf_mtime = 0;
}
/* }}} */

/* {{{ init_ac_trie 
 */
static int init_ac_trie(char *filename, long filesize)
{
	WORDUTIL_G(trie) = ac_trie_create();
	//add trie nodes
    init_trie_by_var();
	//add_trie_by_file(filename, filesize);
	ac_trie_finalize(WORDUTIL_G(trie));
	      
}
/* }}} */

static void init_trie_by_var() {
	int i;
    AC_PATTERN_t patterns[] = {
        PATTERN("百度", "**"),        /* Replace "the " with an empty string */
        PATTERN("滴滴", ""),        /* Replace "滴滴" with an empty string */
        PATTERN("阿里", ""),        /* Replace "阿里" with an empty string */
	};
	const char * texts = "baidu";
	const char * textr = "*";
    for (i = 0; i < PATTERN_COUNT; i++)
    {
        AC_PATTERN_t pattern =  PATTERN(texts, textr);        /* Replace "the " with an empty string */
		printf("p: %d r: %d\r\n", pattern.ptext.length, pattern.rtext.length);
        //AC_PATTERN_t pattern =  PATTERN("baidu", "const");        /* Replace "the " with an empty string */
        if (ac_trie_add (WORDUTIL_G(trie), &pattern, 0) != ACERR_SUCCESS)
            printf("Failed to add pattern \"%.*s\"\n", 
                    (int)pattern.ptext.length, pattern.ptext.astring);
    }
}

static int add_trie_by_file(char *filename, long filesize) {
	FILE *fp;
	size_t ret_code;
	char *buf;
	char *token;
	const char delim[2] = TOKEN_DELIM;
	char *saveptr1, *saveptr2;
	char *pattern;
	char *replace;
	if ((fp = VCWD_FOPEN(filename, "r")) == NULL) {
	    return -1;
	}
	//读取出错
	buf = (char *) emalloc(filesize + 1);
	memset(buf, '0', filesize + 1);
    if ((ret_code = fread(buf, 1, filesize, fp)) != filesize) {
		efree(buf);
	    return -2;
	}
	token = strtok_r(buf, TOKEN_DELIM, &saveptr1);
	while (token != NULL && strstr(token, "=>")) {
		printf("token: %s\r\n", token);
		//todo
		pattern = strtok_r(token, PATTERN_REPLACE_NEEDLE, &saveptr2);
		replace = strtok_r(NULL, PATTERN_REPLACE_NEEDLE, &saveptr2);
		if (pattern == NULL) {
		    continue;
		}
		if (replace == NULL) {
			//int len = strlen(pattern) / sizeof(wchar_t);
			int len = strlen(pattern);
		    replace = emalloc(len + 1);
			memset(replace, '*', len);
			replace[len] = '\0';
		}
		printf("token: %s, pattern: %s, replace: %s\r\n", token, pattern, replace);
        AC_PATTERN_t ac_pattern = PATTERN((const char*)pattern, (const char *)replace);
		if (ac_trie_add(WORDUTIL_G(trie), &ac_pattern, 0) != ACERR_SUCCESS) {
			printf("Failed to add pattern");
		}

        token = strtok_r(NULL, TOKEN_DELIM, &saveptr1);
    }
    efree(buf);
    fclose(fp);
}

static void listener (AC_TEXT_t *text, void *user)
{
    //printf ("%.*s", (int)text->length, text->astring);
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(wordutil)
{
	struct stat buf;
	/* If you have INI entries, uncomment these lines 
	*/
	REGISTER_INI_ENTRIES();
	//记录加载时间
	if (stat(WORDUTIL_G(patterns_path), &buf)) {
	    return FAILURE;
	}
	WORDUTIL_G(pattern_conf_mtime) = buf.st_mtime;
	//加载非法词字典树
	init_ac_trie(WORDUTIL_G(patterns_path), (long)buf.st_size);
	// 保存在全局变量
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(wordutil)
{
	/* uncomment this line if you have INI entries
	*/
	UNREGISTER_INI_ENTRIES();
	ac_trie_release(WORDUTIL_G(trie));
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(wordutil)
{
	struct stat buf;
	//todo 检测字典树文件是否更新，重新加载字典树
	if (stat(WORDUTIL_G(patterns_path), &buf)) {
	    return FAILURE;
	}
	if (buf.st_mtime > WORDUTIL_G(pattern_conf_mtime)) {
	//    printf("%d\r\n", (int)buf.st_mtime);
	    //reload trie
	}
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
	*/
	DISPLAY_INI_ENTRIES();
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
