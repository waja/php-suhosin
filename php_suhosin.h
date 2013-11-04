/*
  +----------------------------------------------------------------------+
  | Suhosin Version 1                                                    |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006 The Hardened-PHP Project                          |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Stefan Esser <sesser@hardened-php.net>                       |
  +----------------------------------------------------------------------+
*/

/* $Id: php_suhosin.h,v 1.48 2006-11-20 15:50:30 sesser Exp $ */

#ifndef PHP_SUHOSIN_H
#define PHP_SUHOSIN_H

#define SUHOSIN_EXT_VERSION  "0.9.13"

/*#define SUHOSIN_DEBUG*/
#define SUHOSIN_LOG "/tmp/suhosin_log.txt"

#ifdef PHP_WIN32
#define SDEBUG
#else

#ifdef SUHOSIN_DEBUG
#define SDEBUG(msg...) \
    {FILE *f;f=fopen(SUHOSIN_LOG, "a+");if(f){fprintf(f,"[%u] ",getpid());fprintf(f, msg);fprintf(f,"\n");fclose(f);}}
#else
#define SDEBUG(...)
#endif    
#endif

extern zend_module_entry suhosin_module_entry;
#define phpext_suhosin_ptr &suhosin_module_entry

#ifdef PHP_WIN32
#define PHP_SUHOSIN_API __declspec(dllexport)
#else
#define PHP_SUHOSIN_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

/*#define STATIC static*/
#define STATIC

#define BYTE unsigned char       /* 8 bits  */
#define WORD unsigned int          /* 32 bits */

PHP_MINIT_FUNCTION(suhosin);
PHP_MSHUTDOWN_FUNCTION(suhosin);
PHP_RINIT_FUNCTION(suhosin);
PHP_RSHUTDOWN_FUNCTION(suhosin);
PHP_MINFO_FUNCTION(suhosin);

ZEND_BEGIN_MODULE_GLOBALS(suhosin)
	zend_uint in_code_type;
	long execution_depth;
	zend_bool simulation;
	zend_bool protectkey;
	zend_bool executor_allow_symlink;
	char *filter_action;
	char *sql_user_prefix;
	char *sql_user_postfix;
	
	long max_execution_depth;
	zend_bool	abort_request;
	long executor_include_max_traversal;
	

	HashTable *include_whitelist;
	HashTable *include_blacklist;

	HashTable *func_whitelist;
	HashTable *func_blacklist;
	HashTable *eval_whitelist;
	HashTable *eval_blacklist;

	zend_bool executor_disable_eval;
	zend_bool executor_disable_emod;


/*	request variables */
	long  max_request_variables;
	long  cur_request_variables;
	long  max_varname_length;
	long  max_totalname_length;
	long  max_value_length;
	long  max_array_depth;
	long  max_array_index_length;
	zend_bool  disallow_nul;
/*	cookie variables */
	long  max_cookie_vars;
	long  cur_cookie_vars;
	long  max_cookie_name_length;
	long  max_cookie_totalname_length;
	long  max_cookie_value_length;
	long  max_cookie_array_depth;
	long  max_cookie_array_index_length;
	zend_bool  disallow_cookie_nul;
/*	get variables */
	long  max_get_vars;
	long  cur_get_vars;
	long  max_get_name_length;
	long  max_get_totalname_length;
	long  max_get_value_length;
	long  max_get_array_depth;
	long  max_get_array_index_length;
	zend_bool  disallow_get_nul;
/*	post variables */
	long  max_post_vars;
	long  cur_post_vars;
	long  max_post_name_length;
	long  max_post_totalname_length;
	long  max_post_value_length;
	long  max_post_array_depth;
	long  max_post_array_index_length;
	zend_bool  disallow_post_nul;

/*	fileupload */
	long  upload_limit;
	long  num_uploads;
	zend_bool  upload_disallow_elf;
	zend_bool  upload_disallow_binary;
	zend_bool  upload_remove_binary;
	char *upload_verification_script;
        
        zend_bool  no_more_variables;
        zend_bool  no_more_get_variables;
        zend_bool  no_more_post_variables;
        zend_bool  no_more_cookie_variables;
        zend_bool  no_more_uploads;



/*	log */
	zend_bool log_use_x_forwarded_for;
	long	log_syslog;
	long	log_syslog_facility;
	long	log_syslog_priority;
	long	log_script;
	long	log_sapi;
	char	*log_scriptname;
	long	log_phpscript;
	char	*log_phpscriptname;
	zend_bool log_phpscript_is_safe;

/*	header handler */
	zend_bool allow_multiheader;

/*	mailprotect */
	long	mailprotect;
	
/*	memory_limit */
	long	memory_limit;
	long 	hard_memory_limit;
	
/*  sqlprotect */
	zend_bool sql_bailout_on_error;


/*	session */
	void	*s_module;
	int 	(*old_s_read)(void **mod_data, const char *key, char **val, int *vallen TSRMLS_DC);
	int	(*old_s_write)(void **mod_data, const char *key, const char *val, const int vallen TSRMLS_DC);
	int	(*old_s_destroy)(void **mod_data, const char *key TSRMLS_DC);
	
	BYTE fi[24],ri[24];
        WORD fkey[120];
        WORD rkey[120];
	
	zend_bool	session_encrypt;
	char*	session_cryptkey;
	zend_bool	session_cryptua;
	zend_bool	session_cryptdocroot;
	long		session_cryptraddr;
	long		session_checkraddr;
	
	long	session_max_id_length;
	
	char*	decrypted_cookie;
    char*	raw_cookie;
	zend_bool	cookie_encrypt;
	char*	cookie_cryptkey;
	zend_bool	cookie_cryptua;
	zend_bool	cookie_cryptdocroot;
	long		cookie_cryptraddr;
	long		cookie_checkraddr;
	HashTable *cookie_plainlist;
	HashTable *cookie_cryptlist;
	
	zend_bool	coredump;
	zend_bool	apc_bug_workaround;

ZEND_END_MODULE_GLOBALS(suhosin)

#ifdef ZTS
#define SUHOSIN_G(v) TSRMG(suhosin_globals_id, zend_suhosin_globals *, v)
#else
#define SUHOSIN_G(v) (suhosin_globals.v)
#endif

#ifndef ZEND_ENGINE_2
#define OnUpdateLong OnUpdateInt
#define zend_symtable_find zend_hash_find
#define zend_symtable_update zend_hash_update
#define zend_symtable_exists zend_hash_exists
#endif


/* Error Constants */
#ifndef S_MEMORY
#define S_MEMORY			(1<<0L)
#define S_MISC				(1<<1L)
#define S_VARS				(1<<2L)
#define S_FILES				(1<<3L)
#define S_INCLUDE			(1<<4L)
#define S_SQL				(1<<5L)
#define S_EXECUTOR			(1<<6L)
#define S_MAIL				(1<<7L)
#define S_SESSION			(1<<8L)
#define S_INTERNAL			(1<<29L)
#define S_ALL (S_MEMORY | S_VARS | S_INCLUDE | S_FILES | S_MAIL | S_SESSION | S_MISC | S_SQL | S_EXECUTOR)
#endif

#define SUHOSIN_NORMAL	0
#define SUHOSIN_EVAL	1

#define SUHOSIN_FLAG_CREATED_BY_EVAL 1
#define SUHOSIN_FLAG_NOT_EVALED_CODE 2

ZEND_EXTERN_MODULE_GLOBALS(suhosin)

static inline char *
suhosin_str_tolower_dup(const char *source, unsigned int length)
{
	register char *dup = estrndup(source, length);
	zend_str_tolower(dup, length);
	return dup;
}

/* functions */
PHP_SUHOSIN_API void suhosin_log(int loglevel, char *fmt, ...);
char *suhosin_encrypt_string(char *str, int len, char *var, int vlen, char *key TSRMLS_DC);
char *suhosin_decrypt_string(char *str, int padded_len, char *var, int vlen, char *key, int *orig_len, int check_ra TSRMLS_DC);
char *suhosin_generate_key(char *key, zend_bool ua, zend_bool dr, long raddr, char *cryptkey TSRMLS_DC);
char *suhosin_cookie_decryptor(TSRMLS_D);
void suhosin_hook_post_handlers(TSRMLS_D);
void suhosin_hook_register_server_variables();
void suhosin_hook_header_handler();
void suhosin_unhook_header_handler();
void suhosin_hook_session(TSRMLS_D);
void suhosin_unhook_session(TSRMLS_D);
void suhosin_hook_crypt();
void suhosin_hook_sha256();
void suhosin_hook_ex_imp();
void suhosin_hook_treat_data();
void suhosin_hook_memory_limit();
void suhosin_hook_execute(TSRMLS_D);
void suhosin_unhook_execute();
void suhosin_aes_gentables();
void suhosin_aes_gkey(int nb,int nk,char *key TSRMLS_DC);
void suhosin_aes_encrypt(char *buff TSRMLS_DC);
void suhosin_aes_decrypt(char *buff TSRMLS_DC);
unsigned int suhosin_input_filter(int arg, char *var, char **val, unsigned int val_len, unsigned int *new_val_len TSRMLS_DC);
void normalize_varname(char *varname);
int suhosin_rfc1867_filter(unsigned int event, void *event_data, void **extra TSRMLS_DC);
void suhosin_bailout(TSRMLS_D);

#endif	/* PHP_SUHOSIN_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
