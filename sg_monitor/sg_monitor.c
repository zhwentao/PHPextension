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
#include "php_sg_monitor.h"
#include "get_cpu.h"

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

static smart_str msgpack_serialize_uri_stat(uri_stat *stat);
static msgpack_object msgpack_unserialize_uri_stat(struct shmcache_value_info *buf);
static int msgpack_serialize_func_stat(func_stat *stat);
static int msgpack_unserialize_func_stat();

static int accumulate_uri(msgpack_object o, uri_stat *sg_uri_stat);
static int gen_shm_uri_key(char *domain_uri_str, char *key);
static int gen_md5_str(char *str, char *md5str);

static zval * request_server_query(char * name, uint len TSRMLS_DC);

//static unsigned int get_cpu_total_occupy();

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
    STD_PHP_INI_ENTRY("sg_monitor.shmcache_conf", "/etc/libshmcache.conf", PHP_INI_SYSTEM, OnUpdateString, shmcache_conf, zend_sg_monitor_globals, sg_monitor_globals)
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
	int shm_result;
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
	//filter
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
	
	SMG(current_uri_str) = SG(request_info).request_uri;
	gettimeofday(&SMG(uri_start_time), NULL);
	SMG(sg_uri_stat).cpu_load= get_cpu_total_occupy();
	SMG(sg_uri_stat).uri_count = 1;
	SMG(sg_uri_stat).failed_count = 0;
}
/* }}} */

/* {{{ uri_monitor_filter
 * filter dommain 
 * set global var uri_need_monitor 1 need monitor, 0 skip monitor
 */
static int uri_monitor_filter()
{
	//SAPI filter, not http request disable monitor
	if (!SG(request_info).request_method) {
		SMG(enable) = 0;
	} else {
        //init domain uri
	    zval* host = request_server_query("HTTP_HOST", sizeof("HTTP_HOST"));
	    zval* uri = request_server_query("REQUEST_URI", sizeof("REQUEST_URI"));
	    SMG(current_domain) = Z_STRVAL_P(host);
	    SMG(current_uri_str) = Z_STRVAL_P(uri);
	    char *domain_uri = (char *)emalloc(P7_STR_LEN(SMG(current_domain)) + P7_STR_LEN(SMG(current_uri_str)) + 1);
	    strcpy(domain_uri, SMG(current_domain));
	    strcat(domain_uri, SMG(current_uri_str));
	    SMG(current_domain_uri) = domain_uri;
	}
	//TODO filter domain
	SMG(uri_need_monitor) = (1 && SMG(enable));
	return SMG(uri_need_monitor);
}
/* }}} */

/* {{{ uri_send_stat
 * send uri statistic to share memory
 * TODO cpu/mem usage, uri error, gen shm key
 */
static int uri_send_stat()
{
    //record uri, cpu, mem, spend_time
	int shm_result;
    int index;
    char *config_filename;
	char md5_key[33];
    struct shmcache_key_info key;
    struct shmcache_value_info shm_value;
	msgpack_object deserialized;

	smart_str value;
	//Calculate current uri stat
	struct timeval end_time;
	gettimeofday(&end_time, NULL);
	//除以1000则进行毫秒计时，如果除以1000000则进行秒级别计时，如果除以1则进行微妙级别计时
	SMG(sg_uri_stat).latency = 1000*(end_time.tv_sec - SMG(uri_start_time).tv_sec) + (end_time.tv_usec - SMG(uri_start_time).tv_usec)/1000; 
	
	//if failed count ++
	SMG(sg_uri_stat).failed_count = 1;

	/**
	 * Get uri stat with key 
	 * Key format: md5(domain_uri_timestamp)
	 */
    gen_shm_uri_key(SMG(current_domain_uri), md5_key);
    key.data = md5_key;
    key.length = P7_STR_LEN(key.data);

	shm_result = shmcache_get(&SMG(monitor_context), &key, &shm_value);

	//Key not exist
	if (shm_result != 0) {
	
	} else {
		//accumulate uri stat
	    deserialized = msgpack_unserialize_uri_stat(&shm_value);
		accumulate_uri(deserialized, &SMG(sg_uri_stat));
	}


	//set value
    value = msgpack_serialize_uri_stat(&SMG(sg_uri_stat));
	shm_result = shmcache_set(&SMG(monitor_context), &key, value.c, value.len, SHMCACHE_NEVER_EXPIRED);

	//test unserialize

	//TODO error log
	if (shm_result != 0) {

    }
}
/* }}} */

/** {{{ msgpack_serialize_uri_stat
 *
 */
static smart_str msgpack_serialize_uri_stat(uri_stat *stat) 
{
    msgpack_sbuffer sbuf;
	smart_str msgstr;
    msgpack_sbuffer_init(&sbuf);

	/* serialize values into the buffer using msgpack_sbuffer_write callback function. */
	msgpack_packer pk;
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 6);
	msgpack_pack_int(&pk, stat->cpu_load);
	msgpack_pack_int(&pk, stat->mem_phys);
	msgpack_pack_int(&pk, stat->mem_virt);
	msgpack_pack_int(&pk, stat->latency);
	msgpack_pack_int(&pk, stat->uri_count);
	msgpack_pack_int(&pk, stat->failed_count);

	smart_str_setl(&msgstr, sbuf.data, sbuf.size);
	return msgstr;
}
/* }}} */

/** {{{ msgpack_unserialize_uri_stat
 *
 */
static msgpack_object msgpack_unserialize_uri_stat(struct shmcache_value_info *buf) 
{
	/* deserialize the buffer into msgpack_object instance. */
	/* deserialized object is valid during the msgpack_zone instance alive. */
	msgpack_zone mempool;
    msgpack_zone_init(&mempool, 2048);

	msgpack_object deserialized;
	msgpack_unpack(buf->data, buf->length, NULL, &mempool, &deserialized);

	/* TODO DEBUG print the deserialized object. */
	//msgpack_object_print(stdout, deserialized);

	msgpack_zone_destroy(&mempool);
	return deserialized;
}
/* }}} */


/** {{{ msgpack_serialize_func_stat
 *
 */
static int msgpack_serialize_func_stat(func_stat *stat)
{}
/* }}} */

/** {{{ msgpack_unserialize_func_stat
 *
 */
static int msgpack_unserialize_func_stat() 
{}
/* }}} */

/** {{{ accumulate_uri
 * Accumulate uri stat
 * [cpu_load, mem_phys, mem_virt, latency, uri_count, failed_count]
 */
static int accumulate_uri(msgpack_object o, uri_stat *uri_stat_ptr) 
{
    if (MSGPACK_OBJECT_ARRAY == o.type && o.via.array.size != 0) {
	    msgpack_object* p = o.via.array.ptr;
		msgpack_object* const pend = o.via.array.ptr + o.via.array.size;

	    int64_t stat[6];
	    int i = 0;
		for (; p < pend; ++p, ++i) {
			if (MSGPACK_OBJECT_NEGATIVE_INTEGER == p->type) {
				stat[i] = p->via.i64;
			} else if (MSGPACK_OBJECT_POSITIVE_INTEGER == p->type) {
				stat[i] = p->via.u64;
			} else {
                stat[i] = 0;
			}
		}
		//max cpu occupy
		if (uri_stat_ptr->cpu_load < stat[0]) {
		    uri_stat_ptr->cpu_load = stat[0];
		}
	    uri_stat_ptr->latency      += stat[3];
	    uri_stat_ptr->uri_count    = stat[4] + 1;
	    uri_stat_ptr->failed_count += stat[5];
	}
}
/* }}} */

/** {{{ gen_md5_str
 * Generate md5 string
 */
static int gen_md5_str(char *str, char *md5)
{
	PHP_MD5_CTX context;
	char md5str[33];
	unsigned char digest[16];
	md5str[0] = '\0';

	PHP_MD5Init(&context);
	PHP_MD5Update(&context, P7_STR(str), P7_STR_LEN(str));
	PHP_MD5Final(digest, &context);
	make_digest_ex(md5str, digest, 16);
    strcpy(md5, md5str);
	return P7_STR_LEN(md5);
}
/* }}} */

/** {{{ gen_shm_uri_key
 * key string format: domain:uri:timesec
 */
static int gen_shm_uri_key(char *domain_uri_str, char *key)
{
    char * tmp;
	char timeslot[6];
	sprintf(timeslot, "%d", (SMG(uri_start_time).tv_sec)%3600);
	size_t tmp_len = P7_STR_LEN(domain_uri_str) + P7_STR_LEN(timeslot) + 1;
    tmp = (char *)emalloc(tmp_len);
    strcpy(tmp, domain_uri_str);
    strncat(tmp, timeslot, tmp_len);
	//PHPWRITE(tmp, P7_STR_LEN(tmp));
	gen_md5_str(tmp, key);
	efree(tmp);
	//PHPWRITE(key, P7_STR_LEN(key));
	return P7_STR_LEN(key);
}

/* }}} */

/** {{{
 * 
 */
static zval * request_server_query(char * name, uint name_len TSRMLS_DC)
{
	zval **carrier = NULL, **ret;

#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4)
	zend_bool jit_initialization = (PG(auto_globals_jit) && !PG(register_globals) && !PG(register_long_arrays));
#else
	zend_bool jit_initialization = PG(auto_globals_jit);
#endif
	if (jit_initialization) {
	    zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC);
	}
	carrier = &PG(http_globals)[TRACK_VARS_SERVER];

	if (zend_hash_find(Z_ARRVAL_PP(carrier), name, P7_STR_LEN(name) + 1, (void **)&ret) == FAILURE) {
	    zval *empty;
		MAKE_STD_ZVAL(empty);
		ZVAL_NULL(empty);
		return empty;
	}

	Z_ADDREF_P(*ret);
	return *ret;
}
/* }}} */

//unsigned int get_cpu_total_occupy()
//{
//    FILE *fd;         //定义文件指针fd
//    char buff[1024] = {0};  //定义局部变量buff数组为char类型大小为1024
//    total_cpu_occupy_t t;
//    fd = fopen ("/proc/stat", "r"); //以R读的方式打开stat文件再赋给指针fd
//    fgets (buff, sizeof(buff), fd); //从fd文件中读取长度为buff的字符串再存到起始地址为buff这个空间里
//    /*下面是将buff的字符串根据参数format后转换为数据的结果存入相应的结构体参数 */
//    char name[16];//暂时用来存放字符串
//    sscanf (buff, "%s %u %u %u %u", name, &t.user, &t.nice,&t.system, &t.idle);
//    
//    fclose(fd);     //关闭文件fd
//    return ((t.user + t.nice + t.system)*100)/(t.user + t.nice + t.system + t.idle);
//}

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
