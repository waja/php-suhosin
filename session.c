/*
  +----------------------------------------------------------------------+
  | Suhosin Version 1                                                    |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2007 The Hardened-PHP Project                     |
  | Copyright (c) 2007 SektionEins GmbH                                  |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Stefan Esser <sesser@sektioneins.de>                         |
  +----------------------------------------------------------------------+
*/
/*
  $Id: session.c,v 1.1.1.1 2007-11-28 01:15:35 sesser Exp $ 
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "TSRM.h"
#include "SAPI.h"
#include "php_ini.h"
#include "php_suhosin.h"
#include "ext/standard/base64.h"
#include "sha256.h"

#define PS_OPEN_ARGS void **mod_data, const char *save_path, const char *session_name TSRMLS_DC
#define PS_CLOSE_ARGS void **mod_data TSRMLS_DC
#define PS_READ_ARGS void **mod_data, const char *key, char **val, int *vallen TSRMLS_DC
#define PS_WRITE_ARGS void **mod_data, const char *key, const char *val, const int vallen TSRMLS_DC
#define PS_DESTROY_ARGS void **mod_data, const char *key TSRMLS_DC
#define PS_GC_ARGS void **mod_data, int maxlifetime, int *nrdels TSRMLS_DC
#define PS_CREATE_SID_ARGS void **mod_data, int *newlen TSRMLS_DC

typedef struct ps_module_struct {
	const char *s_name;
	int (*s_open)(PS_OPEN_ARGS);
	int (*s_close)(PS_CLOSE_ARGS);
	int (*s_read)(PS_READ_ARGS);
	int (*s_write)(PS_WRITE_ARGS);
	int (*s_destroy)(PS_DESTROY_ARGS);
	int (*s_gc)(PS_GC_ARGS);
	char *(*s_create_sid)(PS_CREATE_SID_ARGS);
} ps_module;

typedef enum {
	php_session_disabled,
	php_session_none,
	php_session_active
} php_session_status;

#define PS_SERIALIZER_ENCODE_ARGS char **newstr, int *newlen TSRMLS_DC
#define PS_SERIALIZER_DECODE_ARGS const char *val, int vallen TSRMLS_DC

typedef struct ps_serializer_struct {
	const char *name;
	int (*encode)(PS_SERIALIZER_ENCODE_ARGS);
	int (*decode)(PS_SERIALIZER_DECODE_ARGS);
} ps_serializer;

typedef struct _php_ps_globals_43_44 {
	char *save_path;
	char *session_name;
	char *id;
	char *extern_referer_chk;
	char *entropy_file;
	char *cache_limiter;
	long entropy_length;
	long cookie_lifetime;
	char *cookie_path;
	char *cookie_domain;
	zend_bool  cookie_secure;
	ps_module *mod;
	void *mod_data;
	php_session_status session_status;
	long gc_probability;
	long gc_divisor;
	long gc_maxlifetime;
	int module_number;
	long cache_expire;
	zend_bool bug_compat; /* Whether to behave like PHP 4.2 and earlier */
	zend_bool bug_compat_warn; /* Whether to warn about it */
	const struct ps_serializer_struct *serializer;
	zval *http_session_vars;
	zend_bool auto_start;
	zend_bool use_cookies;
	zend_bool use_only_cookies;
	zend_bool use_trans_sid;	/* contains the INI value of whether to use trans-sid */
	zend_bool apply_trans_sid;	/* whether or not to enable trans-sid for the current request */
	int send_cookie;
	int define_sid;
} php_ps_globals_43_44;

typedef struct _php_ps_globals_50_51 {
	char *save_path;
	char *session_name;
	char *id;
	char *extern_referer_chk;
	char *entropy_file;
	char *cache_limiter;
	long entropy_length;
	long cookie_lifetime;
	char *cookie_path;
	char *cookie_domain;
	zend_bool  cookie_secure;
	ps_module *mod;
	void *mod_data;
	php_session_status session_status;
	long gc_probability;
	long gc_divisor;
	long gc_maxlifetime;
	int module_number;
	long cache_expire;
	zend_bool bug_compat; /* Whether to behave like PHP 4.2 and earlier */
	zend_bool bug_compat_warn; /* Whether to warn about it */
	const struct ps_serializer_struct *serializer;
	zval *http_session_vars;
	zend_bool auto_start;
	zend_bool use_cookies;
	zend_bool use_only_cookies;
	zend_bool use_trans_sid;	/* contains the INI value of whether to use trans-sid */
	zend_bool apply_trans_sid;	/* whether or not to enable trans-sid for the current request */

	long hash_func;
	long hash_bits_per_character;
	int send_cookie;
	int define_sid;
} php_ps_globals_50_51;

typedef struct _php_ps_globals_52_60 {
	char *save_path;
	char *session_name;
	char *id;
	char *extern_referer_chk;
	char *entropy_file;
	char *cache_limiter;
	long entropy_length;
	long cookie_lifetime;
	char *cookie_path;
	char *cookie_domain;
	zend_bool  cookie_secure;
	zend_bool  cookie_httponly;
	ps_module *mod;
	void *mod_data;
	php_session_status session_status;
	long gc_probability;
	long gc_divisor;
	long gc_maxlifetime;
	int module_number;
	long cache_expire;
	zend_bool bug_compat; /* Whether to behave like PHP 4.2 and earlier */
	zend_bool bug_compat_warn; /* Whether to warn about it */
	const struct ps_serializer_struct *serializer;
	zval *http_session_vars;
	zend_bool auto_start;
	zend_bool use_cookies;
	zend_bool use_only_cookies;
	zend_bool use_trans_sid;	/* contains the INI value of whether to use trans-sid */
	zend_bool apply_trans_sid;	/* whether or not to enable trans-sid for the current request */

	long hash_func;
	long hash_bits_per_character;
	int send_cookie;
	int define_sid;
	zend_bool invalid_session_id;	/* allows the driver to report about an invalid session id and request id regeneration */
} php_ps_globals_52_60;


#ifdef ZTS
static ts_rsrc_id session_globals_id = 0;
# if PHP_MAJOR_VERSION > 5 || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 2)
#  define SESSION_G(v) TSRMG(session_globals_id, php_ps_globals_52_60 *, v)
# elif (PHP_MAJOR_VERSION == 5)
#  define SESSION_G(v) TSRMG(session_globals_id, php_ps_globals_50_51 *, v)
# elif (PHP_MAJOR_VERSION == 4 && PHP_MINOR_VERSION >= 3)
#  define SESSION_G(v) TSRMG(session_globals_id, php_ps_globals_43_44 *, v)
# else 
    UNSUPPORTED PHP VERSION
# endif
#else
# if PHP_MAJOR_VERSION > 5 || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 2)
static php_ps_globals_52_60 *session_globals = NULL;
# elif (PHP_MAJOR_VERSION == 5)
static php_ps_globals_50_51 *session_globals = NULL;
# elif (PHP_MAJOR_VERSION == 4 && PHP_MINOR_VERSION >= 3)
static php_ps_globals_43_44 *session_globals = NULL;
# else 
    UNSUPPORTED PHP VERSION
# endif
#define SESSION_G(v) (session_globals->v)
#endif

void suhosin_get_ipv4(char *buf TSRMLS_DC)
{
	char *raddr = sapi_getenv("REMOTE_ADDR", sizeof("REMOTE_ADDR")-1 TSRMLS_CC);
	int i;


	if (raddr == NULL) {
		memset(buf, 0, 4);
		return;
	}
	
	for (i=0; i<4; i++) {
		if (raddr[0] == 0) {
			buf[i] = 0;
		} else {
			buf[i] = strtol(raddr, &raddr, 10);
			if (raddr[0] == '.') {
				raddr++;
			}
		}
	}
}

char *suhosin_encrypt_string(char *str, int len, char *var, int vlen, char *key TSRMLS_DC)
{
    int padded_len, i, slen;
    unsigned char *crypted, *tmp;
	unsigned int check = 0x13579BDF;
        
    if (str == NULL) {
        return NULL;
    }
	if (len == 0) {
		return estrndup("", 0);
	}


    suhosin_aes_gkey(4,8,key TSRMLS_CC);

    padded_len = ((len+15) & ~0xF);
    crypted = emalloc(16+padded_len+1);
	memset(crypted, 0xff, 16+padded_len+1);
    memcpy(crypted+16, str, len+1);

	/* calculate check value */
	for (i = 0; i<vlen; i++) {
		check = (check << 3) | (check >> (32-3));
		check += check << 1;
		check ^= (unsigned char)var[i];
	}
	for (i = 0; i<len; i++) {
		check = (check << 3) | (check >> (32-3));
		check += check << 1;
		check ^= (unsigned char)str[i];
	}
	
	/* store ip value */
	suhosin_get_ipv4(crypted+4 TSRMLS_CC);
	
	/* store check value */
	crypted[8] = check & 0xff;
	crypted[9] = (check >> 8) & 0xff;
	crypted[10] = (check >> 16) & 0xff;
	crypted[11] = (check >> 24) & 0xff;

	/* store original length */
	crypted[12] = len & 0xff;
	crypted[13] = (len >> 8) & 0xff;
	crypted[14] = (len >> 16) & 0xff;
	crypted[15] = (len >> 24) & 0xff;
        
    for (i=0, tmp=crypted; i<padded_len+16; i+=16, tmp+=16) {
        if (i > 0) {
      	    int j;
            for (j=0; j<16; j++) tmp[j] ^= tmp[j-16];
        }
        suhosin_aes_encrypt((char *)tmp TSRMLS_CC);
    }
        
    tmp = php_base64_encode(crypted, padded_len+16, NULL);
    efree(crypted);
    slen=strlen((char *)tmp);
	for (i=0; i<slen; i++) {
	    switch (tmp[i]) {
		case '/': tmp[i]='-'; break;
		case '=': tmp[i]='.'; break;
		case '+': tmp[i]='_'; break;
	    }
	}
    return (char *)tmp;
}

char *suhosin_decrypt_string(char *str, int padded_len, char *var, int vlen, char *key, int *orig_len, int check_ra TSRMLS_DC)
{
    int len, i, o_len, invalid = 0;
    unsigned char *decrypted, *tmp;
	unsigned int check = 0x13579BDF;
    char buf[4];
        
    if (str == NULL) {
        return NULL;
    }
	
	if (padded_len == 0) {
		if (orig_len) {
			*orig_len = 0;
		}
		return estrndup("", 0);
	}
    suhosin_aes_gkey(4,8,key TSRMLS_CC);

	for (i=0; i<padded_len; i++) {
	    switch (str[i]) {
		case '-': str[i]='/'; break;
		case '.': str[i]='='; break;
		case '_': str[i]='+'; break;
	    }
	}
        
    decrypted = php_base64_decode((unsigned char *)str, padded_len, &len);
	if (decrypted == NULL || len < 2*16 || (len % 16) != 0) {
error_out:
		if (decrypted != NULL) {
			efree(decrypted);
		}
		if (orig_len) {
			*orig_len = 0;
		}
		return NULL;
	}
        
    for (i=len-16, tmp=decrypted+i; i>=0; i-=16, tmp-=16) {
        suhosin_aes_decrypt((char *)tmp TSRMLS_CC);
        if (i > 0) {
            int j;
            for (j=0; j<16; j++) tmp[j] ^= tmp[j-16];
        }
    }
	
	/* retrieve orig_len */
	o_len = decrypted[15];
	o_len <<= 8;
	o_len |= decrypted[14];
	o_len <<= 8;
	o_len |= decrypted[13];
	o_len <<= 8;
	o_len |= decrypted[12];
	
	if (o_len < 0 || o_len > len-16) {
		goto error_out;
	}

	/* calculate check value */
	for (i = 0; i<vlen; i++) {
		check = (check << 3) | (check >> (32-3));
		check += check << 1;
		check ^= (unsigned char)var[i];
	}
	for (i = 0; i<o_len; i++) {
		check = (check << 3) | (check >> (32-3));
		check += check << 1;
		check ^= decrypted[16+i];
	}
	
	/* check value */
	invalid = (decrypted[8] != (check & 0xff)) || 
	           (decrypted[9] != ((check >> 8) & 0xff)) || 
			   (decrypted[10] != ((check >> 16) & 0xff)) ||	
			   (decrypted[11] != ((check >> 24) & 0xff));
	
	/* check IP */
	if (check_ra > 0) {
		if (check_ra > 4) {
			check_ra = 4;
		}
		suhosin_get_ipv4(&buf TSRMLS_CC);
		if (memcmp(buf, decrypted+4, check_ra) != 0) {
			goto error_out;
		}
	}
	
	if (invalid) {
		goto error_out;
	}
	
	if (orig_len) {
		*orig_len = o_len;
	}
	
	memmove(decrypted, decrypted+16, o_len);
	decrypted[o_len] = 0;
	/* we do not realloc() here because 16 byte less 
	   is simply not worth the overhead */	
	return (char *)decrypted;
}

char *suhosin_generate_key(char *key, zend_bool ua, zend_bool dr, long raddr, char *cryptkey TSRMLS_DC)
{
	char *_ua = NULL;
	char *_dr = NULL;
	char *_ra = NULL;
	suhosin_SHA256_CTX ctx;
	
	if (ua) {
		_ua = sapi_getenv("HTTP_USER_AGENT", sizeof("HTTP_USER_AGENT")-1 TSRMLS_CC);
	}
	
	if (dr) {
		_dr = sapi_getenv("DOCUMENT_ROOT", sizeof("DOCUMENT_ROOT")-1 TSRMLS_CC);
	}
	
	if (raddr > 0) {
		_ra = sapi_getenv("REMOTE_ADDR", sizeof("REMOTE_ADDR")-1 TSRMLS_CC);
	}
	
	SDEBUG("(suhosin_generate_key) KEY: %s - UA: %s - DR: %s - RA: %s", key,_ua,_dr,_ra);
	
	suhosin_SHA256Init(&ctx);
	if (key == NULL) {
		suhosin_SHA256Update(&ctx, (unsigned char*)"D3F4UL7", sizeof("D3F4UL7"));
	} else {
		suhosin_SHA256Update(&ctx, (unsigned char*)key, strlen(key));
	}
	if (_ua) {
		suhosin_SHA256Update(&ctx, (unsigned char*)_ua, strlen(_ua));
	}
	if (_dr) {
		suhosin_SHA256Update(&ctx, (unsigned char*)_dr, strlen(_dr));
	}
	if (_ra) {
		if (raddr >= 4) {
			suhosin_SHA256Update(&ctx, (unsigned char*)_ra, strlen(_ra));
		} else {
			long dots = 0;
			char *tmp = _ra;
			
			while (*tmp) {
				if (*tmp == '.') {
					dots++;
					if (dots == raddr) {
						break;
					}
				}
				tmp++;
			}
			suhosin_SHA256Update(&ctx, (unsigned char*)_ra, tmp-_ra);
		}
	}
	suhosin_SHA256Final((unsigned char *)cryptkey, &ctx);
	cryptkey[32] = 0; /* uhmm... not really a string */
	
	return cryptkey;
}


static int (*old_OnUpdateSaveHandler)(zend_ini_entry *entry, char *new_value, uint new_value_length, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage TSRMLS_DC) = NULL;
static int (*old_SessionRINIT)(INIT_FUNC_ARGS) = NULL;

static int suhosin_hook_s_read(void **mod_data, const char *key, char **val, int *vallen TSRMLS_DC)
{
	int r;
	
	int i;char *v,*KEY=(char *)key;

	/* protect session vars */
/*	if (SESSION_G(http_session_vars) && SESSION_G(http_session_vars)->type == IS_ARRAY) {
		SESSION_G(http_session_vars)->refcount++;
	}*/
	
	/* protect dumb session handlers */
	if (key == NULL || !key[0] || *mod_data == NULL) {
regenerate:
		SDEBUG("regenerating key is %s", key);
		KEY = SESSION_G(id) = SESSION_G(mod)->s_create_sid(&SESSION_G(mod_data), NULL TSRMLS_CC);
		SESSION_G(send_cookie) = 1;
	} else if (strlen(key) > SUHOSIN_G(session_max_id_length)) {
		suhosin_log(S_SESSION, "session id ('%s') exceeds maximum length - regenerating", KEY);
		if (!SUHOSIN_G(simulation)) {
			goto regenerate;
		}
	}
#if (PHP_MAJOR_VERSION < 5) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 2)
	  else if (strpbrk(KEY, "\r\n\t <>'\"\\")) {
		suhosin_log(S_SESSION, "session id ('%s') contains invalid chars - regenerating", KEY);
		if (!SUHOSIN_G(simulation)) {
			goto regenerate;
		}
	}
#endif

	r = SUHOSIN_G(old_s_read)(mod_data, KEY, val, vallen TSRMLS_CC);

	if (r == SUCCESS && SUHOSIN_G(session_encrypt) && *vallen > 0) {
		char cryptkey[33];
	
                SUHOSIN_G(do_not_scan) = 1;
		suhosin_generate_key(SUHOSIN_G(session_cryptkey), SUHOSIN_G(session_cryptua), SUHOSIN_G(session_cryptdocroot), SUHOSIN_G(session_cryptraddr), (char *)&cryptkey TSRMLS_CC);
	
		v = *val;
		i = *vallen;
		*val = suhosin_decrypt_string(v, i, "", 0, (char *)&cryptkey, vallen, SUHOSIN_G(session_checkraddr) TSRMLS_CC);
                SUHOSIN_G(do_not_scan) = 0;
        if (*val == NULL) {
            *val = estrndup("", 0);
            *vallen = 0;
        }
		efree(v);
	}
	
	return r;
}

static int suhosin_hook_s_write(void **mod_data, const char *key, const char *val, const int vallen TSRMLS_DC)
{
	int r;
/*	int nullify = 0;*/
	char *v = (char *)val;

	/* protect dumb session handlers */
	if (key == NULL || !key[0] || val == NULL || strlen(key) > SUHOSIN_G(session_max_id_length) || *mod_data == NULL) {
		r = FAILURE;
		goto return_write;
	}
	
	r = vallen;

	if (r > 0 && SUHOSIN_G(session_encrypt)) {
		char cryptkey[33];

                SUHOSIN_G(do_not_scan) = 1;
	
		suhosin_generate_key(SUHOSIN_G(session_cryptkey), SUHOSIN_G(session_cryptua), SUHOSIN_G(session_cryptdocroot), SUHOSIN_G(session_cryptraddr), (char *)&cryptkey TSRMLS_CC);
		
		v = suhosin_encrypt_string(v, vallen, "", 0, (char *)&cryptkey TSRMLS_CC);
		
		SUHOSIN_G(do_not_scan) = 0;
		r = strlen(v);
	}

	r = SUHOSIN_G(old_s_write)(mod_data, key, v, r TSRMLS_CC);
	
return_write:
	/* protect session vars */
/*	if (SESSION_G(http_session_vars) && SESSION_G(http_session_vars)->type == IS_ARRAY) {
		if (SESSION_G(http_session_vars)->refcount==1) {
			nullify = 1;
		}
		zval_ptr_dtor(&SESSION_G(http_session_vars));
		if (nullify) {
			suhosin_log(S_SESSION, "possible session variables double free attack stopped");
			SESSION_G(http_session_vars) = NULL;
		}
	}*/

	return r;
}

static int suhosin_hook_s_destroy(void **mod_data, const char *key TSRMLS_DC)
{
	int r;

	/* protect dumb session handlers */
	if (key == NULL || !key[0] || strlen(key) > SUHOSIN_G(session_max_id_length) || *mod_data == NULL) {
		return FAILURE;
	}
	
	r = SUHOSIN_G(old_s_destroy)(mod_data, key TSRMLS_CC);
	
	return r;
}

static void suhosin_hook_session_module(TSRMLS_D)
{
	ps_module *old_mod = SESSION_G(mod), *mod;

	if (old_mod == NULL || SUHOSIN_G(s_module) == old_mod) {
		return;
	}
	if (SUHOSIN_G(s_module) == NULL) {
		SUHOSIN_G(s_module) = mod = malloc(sizeof(ps_module));
		if (mod == NULL) {
			return;
		}
	}
	mod = SUHOSIN_G(s_module);
	memcpy(mod, old_mod, sizeof(ps_module));
	
	SUHOSIN_G(old_s_read) = mod->s_read;
	mod->s_read = suhosin_hook_s_read;
	SUHOSIN_G(old_s_write) = mod->s_write;
	mod->s_write = suhosin_hook_s_write;
	SUHOSIN_G(old_s_destroy) = mod->s_destroy;
	mod->s_destroy = suhosin_hook_s_destroy;
	
	SESSION_G(mod) = mod;
}

static PHP_INI_MH(suhosin_OnUpdateSaveHandler)
{
	int r;

	r = old_OnUpdateSaveHandler(entry, new_value, new_value_length, mh_arg1, mh_arg2, mh_arg3, stage TSRMLS_CC);
	
	suhosin_hook_session_module(TSRMLS_C);
	
	return r;
}


static int suhosin_hook_session_RINIT(INIT_FUNC_ARGS)
{
	if (SESSION_G(mod) == NULL) {
		char *value = zend_ini_string("session.save_handler", sizeof("session.save_handler"), 0);
		
		if (value) {
			suhosin_OnUpdateSaveHandler(NULL, value, strlen(value), NULL, NULL, NULL, 0 TSRMLS_CC);
		}
	}
	return old_SessionRINIT(INIT_FUNC_ARGS_PASSTHRU);
}

void suhosin_hook_session(TSRMLS_D)
{
	zend_ini_entry *ini_entry;
	zend_module_entry *module;
#ifdef ZTS
    	ts_rsrc_id *ps_globals_id_ptr;
#endif
	
	if (zend_hash_find(&module_registry, "session", sizeof("session"), (void**)&module) == FAILURE) {
		return;
	}
    /* retrieve globals from module entry struct if possible */
#if PHP_VERSION_ID >= 50200
#ifdef ZTS
    if (session_globals_id == 0) {
        session_globals_id = *module->globals_id_ptr;
    }
#else
    if (session_globals == NULL) {
        session_globals = module->globals_ptr;
    }
#endif
#else
	/* retrieve globals from symbols if PHP version is old */
#ifdef ZTS
	if (session_globals_id == 0) {
		ps_globals_id_ptr = DL_FETCH_SYMBOL(module->handle, "ps_globals_id");
		if (ps_globals_id_ptr == NULL) {
			ps_globals_id_ptr = DL_FETCH_SYMBOL(module->handle, "_ps_globals_id");
		}
		if (ps_globals_id_ptr == NULL) {
			return;
		}
		
		session_globals_id = *ps_globals_id_ptr;
	}
#else
	if (session_globals == NULL) {
		session_globals = DL_FETCH_SYMBOL(module->handle, "ps_globals");
		if (session_globals == NULL) {
			session_globals = DL_FETCH_SYMBOL(module->handle, "_ps_globals");
		}
		if (session_globals == NULL) {
			return;
		}
	}
#endif
#endif
	if (old_OnUpdateSaveHandler != NULL) {
		return;
	}
	
	/* hook request startup function of session module */
	old_SessionRINIT = module->request_startup_func;
	module->request_startup_func = suhosin_hook_session_RINIT;
	
	/* retrieve pointer to session.save_handler ini entry */
	if (zend_hash_find(EG(ini_directives), "session.save_handler", sizeof("session.save_handler"), (void **) &ini_entry) == FAILURE) {
		return;
	}
	SUHOSIN_G(s_module) = NULL;

	/* replace OnUpdateMemoryLimit handler */
	old_OnUpdateSaveHandler = ini_entry->on_modify;
	ini_entry->on_modify = suhosin_OnUpdateSaveHandler;
	
	suhosin_hook_session_module(TSRMLS_C);
}

void suhosin_unhook_session(TSRMLS_D)
{
	if (old_OnUpdateSaveHandler != NULL) {
		zend_ini_entry *ini_entry;
		
		/* retrieve pointer to session.save_handler ini entry */
		if (zend_hash_find(EG(ini_directives), "session.save_handler", sizeof("session.save_handler"), (void **) &ini_entry) == FAILURE) {
			return;
		}
		ini_entry->on_modify = old_OnUpdateSaveHandler;
	
		old_OnUpdateSaveHandler = NULL;	
	}

}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
