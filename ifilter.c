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
/*
  $Id: ifilter.c,v 1.6 2006-10-25 21:38:00 sesser Exp $ 
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_suhosin.h"
#include "php_variables.h"


static void (*orig_register_server_variables)(zval *track_vars_array TSRMLS_DC) = NULL;

#ifdef ZEND_ENGINE_2
#define HASH_HTTP_GET_VARS      0x2095733f
#define HASH_HTTP_POST_VARS     0xbfee1265
#define HASH_HTTP_COOKIE_VARS   0xaaca9d99
#define HASH_HTTP_ENV_VARS      0x1fe186a8
#define HASH_HTTP_SERVER_VARS   0xc987afd6
#define HASH_HTTP_SESSION_VARS  0x7aba0d43
#define HASH_HTTP_POST_FILES    0x98eb1ddc
#define HASH_HTTP_RAW_POST_DATA 0xdd633fec
#else
#define HASH_HTTP_GET_VARS      0x8d8645bd
#define HASH_HTTP_POST_VARS     0x7c699bf3
#define HASH_HTTP_COOKIE_VARS   0x93ad0d6f
#define HASH_HTTP_ENV_VARS      0x84da3016
#define HASH_HTTP_SERVER_VARS   0x6dbf964e
#define HASH_HTTP_SESSION_VARS  0x322906f5
#define HASH_HTTP_POST_FILES    0xe4e4ce70
#define HASH_HTTP_RAW_POST_DATA 0xe6137a0e
#endif


/* {{{ normalize_varname
 */
void normalize_varname(char *varname)
{
	char *s=varname, *index=NULL, *indexend=NULL, *p;
	
	/* overjump leading space */
	while (*s == ' ') {
		s++;
	}
	
	/* and remove it */
	if (s != varname) {
		memmove(varname, s, strlen(s)+1);
	}

	for (p=varname; *p && *p != '['; p++) {
		switch(*p) {
			case ' ':
			case '.':
				*p='_';
				break;
		}
	}

	/* find index */
	index = strchr(varname, '[');
	if (index) {
		index++;
		s=index;
	} else {
		return;
	}

	/* done? */
	while (index) {

		while (*index == ' ' || *index == '\r' || *index == '\n' || *index=='\t') {
			index++;
		}
		indexend = strchr(index, ']');
		indexend = indexend ? indexend + 1 : index + strlen(index);
		
		if (s != index) {
			memmove(s, index, strlen(index)+1);
			s += indexend-index;
		} else {
			s = indexend;
		}

		if (*s == '[') {
			s++;
			index = s;
		} else {
			index = NULL;
		}	
	}
	*s++='\0';
}
/* }}} */


/* {{{ suhosin_register_server_variables
 */
void suhosin_register_server_variables(zval *track_vars_array TSRMLS_DC)
{
        HashTable *svars;
        int retval, failure=0;
        
        orig_register_server_variables(track_vars_array TSRMLS_CC);

        svars = Z_ARRVAL_P(track_vars_array);
        
	if (!SUHOSIN_G(simulation)) {
    		retval = zend_hash_del_key_or_index(svars, "HTTP_GET_VARS", sizeof("HTTP_GET_VARS"), HASH_HTTP_GET_VARS, HASH_DEL_INDEX);
    		if (retval == SUCCESS) failure = 1;
    		retval = zend_hash_del_key_or_index(svars, "HTTP_POST_VARS", sizeof("HTTP_POST_VARS"), HASH_HTTP_POST_VARS, HASH_DEL_INDEX);
    		if (retval == SUCCESS) failure = 1;
    		retval = zend_hash_del_key_or_index(svars, "HTTP_COOKIE_VARS", sizeof("HTTP_COOKIE_VARS"), HASH_HTTP_COOKIE_VARS, HASH_DEL_INDEX);
    		if (retval == SUCCESS) failure = 1;
    		retval = zend_hash_del_key_or_index(svars, "HTTP_ENV_VARS", sizeof("HTTP_ENV_VARS"), HASH_HTTP_ENV_VARS, HASH_DEL_INDEX);
    		if (retval == SUCCESS) failure = 1;
    		retval = zend_hash_del_key_or_index(svars, "HTTP_SERVER_VARS", sizeof("HTTP_SERVER_VARS"), HASH_HTTP_SERVER_VARS, HASH_DEL_INDEX);
    		if (retval == SUCCESS) failure = 1;
    		retval = zend_hash_del_key_or_index(svars, "HTTP_SESSION_VARS", sizeof("HTTP_SESSION_VARS"), HASH_HTTP_SESSION_VARS, HASH_DEL_INDEX);
    		if (retval == SUCCESS) failure = 1;
    		retval = zend_hash_del_key_or_index(svars, "HTTP_POST_FILES", sizeof("HTTP_POST_FILES"), HASH_HTTP_POST_FILES, HASH_DEL_INDEX);
		if (retval == SUCCESS) failure = 1;
    		retval = zend_hash_del_key_or_index(svars, "HTTP_RAW_POST_DATA", sizeof("HTTP_RAW_POST_DATA"), HASH_HTTP_RAW_POST_DATA, HASH_DEL_INDEX);
    		if (retval == SUCCESS) failure = 1;
	} else {
		retval = zend_hash_exists(svars, "HTTP_GET_VARS", sizeof("HTTP_GET_VARS"));
		retval+= zend_hash_exists(svars, "HTTP_POST_VARS", sizeof("HTTP_POST_VARS"));
		retval+= zend_hash_exists(svars, "HTTP_COOKIE_VARS", sizeof("HTTP_COOKIE_VARS"));
		retval+= zend_hash_exists(svars, "HTTP_ENV_VARS", sizeof("HTTP_ENV_VARS"));
		retval+= zend_hash_exists(svars, "HTTP_SERVER_VARS", sizeof("HTTP_SERVER_VARS"));
		retval+= zend_hash_exists(svars, "HTTP_SESSION_VARS", sizeof("HTTP_SESSION_VARS"));
		retval+= zend_hash_exists(svars, "HTTP_POST_FILES", sizeof("HTTP_POST_FILES"));
		retval+= zend_hash_exists(svars, "HTTP_RAW_POST_DATAS", sizeof("HTTP_RAW_POST_DATA"));
		if (retval > 0) failure = 1;
	}
        
        if (failure) {
                suhosin_log(S_VARS, "Attacker tried to overwrite a superglobal through a HTTP header");
        }
	
	if (SUHOSIN_G(raw_cookie)) {
        zval *z;
        MAKE_STD_ZVAL(z);
		ZVAL_STRING(z, SUHOSIN_G(raw_cookie), 1);
		zend_hash_add(svars, "RAW_HTTP_COOKIE", sizeof("RAW_HTTP_COOKIE"), (void **)&z, sizeof(zval *), NULL);
    }
    if (SUHOSIN_G(decrypted_cookie)) {
        zval *z;
		MAKE_STD_ZVAL(z);
		ZVAL_STRING(z, SUHOSIN_G(decrypted_cookie), 0);
		zend_hash_update(svars, "HTTP_COOKIE", sizeof("HTTP_COOKIE"), (void **)&z, sizeof(zval *), NULL);
		SUHOSIN_G(decrypted_cookie) = NULL;
	}
}
/* }}} */


/* {{{ suhosin_input_filter
 */
unsigned int suhosin_input_filter(int arg, char *var, char **val, unsigned int val_len, unsigned int *new_val_len TSRMLS_DC)
{
	char *index, *prev_index = NULL;
	unsigned int var_len, total_len, depth = 0;

	/* Drop this variable if the limit was reached */
        switch (arg) {
            case PARSE_GET:
                        if (SUHOSIN_G(no_more_get_variables)) {
                                return 0;
                        }
                        break;
            case PARSE_POST:
                        if (SUHOSIN_G(no_more_post_variables)) {
                                return 0;
                        }
                        break;
            case PARSE_COOKIE:
                        if (SUHOSIN_G(no_more_cookie_variables)) {
                                return 0;
                        }
                        break;
            default:    /* we do not want to protect parse_str() and friends */
	                if (new_val_len) {
		                *new_val_len = val_len;
	                }
                        return 1;
        }
        if (SUHOSIN_G(no_more_variables)) {
                return 0;
        }
        
        /* Drop this variable if the limit is now reached */
	if (SUHOSIN_G(max_request_variables) && SUHOSIN_G(max_request_variables) <= SUHOSIN_G(cur_request_variables)) {
		suhosin_log(S_VARS, "configured request variable limit exceeded - dropped variable '%s'", var);
		if (!SUHOSIN_G(simulation)) {
            		SUHOSIN_G(no_more_variables) = 1;
			return 0;
		}
	}
	switch (arg) {
	    case PARSE_GET:
			if (SUHOSIN_G(max_get_vars) && SUHOSIN_G(max_get_vars) <= SUHOSIN_G(cur_get_vars)) {
				suhosin_log(S_VARS, "configured GET variable limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
                            		SUHOSIN_G(no_more_get_variables) = 1;
					return 0;
				}
			}
			break;
	    case PARSE_COOKIE:
			if (SUHOSIN_G(max_cookie_vars) && SUHOSIN_G(max_cookie_vars) <= SUHOSIN_G(cur_cookie_vars)) {
				suhosin_log(S_VARS, "configured COOKIE variable limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
                            		SUHOSIN_G(no_more_cookie_variables) = 1;
					return 0;
				}
			}
			break;
	    case PARSE_POST:
			if (SUHOSIN_G(max_post_vars) && SUHOSIN_G(max_post_vars) <= SUHOSIN_G(cur_post_vars)) {
				suhosin_log(S_VARS, "configured POST variable limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
                            		SUHOSIN_G(no_more_post_variables) = 1;
                            		return 0;
				}
			}
			break;
	}
	
	
	/* Drop this variable if it exceeds the value length limit */
	if (SUHOSIN_G(max_value_length) && SUHOSIN_G(max_value_length) < val_len) {
		suhosin_log(S_VARS, "configured request variable value length limit exceeded - dropped variable '%s'", var);
		if (!SUHOSIN_G(simulation)) {
			return 0;
		}
	}
	switch (arg) {
	    case PARSE_GET:
			if (SUHOSIN_G(max_get_value_length) && SUHOSIN_G(max_get_value_length) < val_len) {
				suhosin_log(S_VARS, "configured GET variable value length limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
					return 0;
				}
			}
			break;
	    case PARSE_COOKIE:
			if (SUHOSIN_G(max_cookie_value_length) && SUHOSIN_G(max_cookie_value_length) < val_len) {
				suhosin_log(S_VARS, "configured COOKIE variable value length limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
					return 0;
				}
			}
			break;
	    case PARSE_POST:
			if (SUHOSIN_G(max_post_value_length) && SUHOSIN_G(max_post_value_length) < val_len) {
				suhosin_log(S_VARS, "configured POST variable value length limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
					return 0;
				}
			}
			break;
	}
	
	/* Normalize the variable name */
	normalize_varname(var);
	
	/* Find length of variable name */
	index = strchr(var, '[');
	total_len = strlen(var);
	var_len = index ? index-var : total_len;
	
	/* Drop this variable if it exceeds the varname/total length limit */
	if (SUHOSIN_G(max_varname_length) && SUHOSIN_G(max_varname_length) < var_len) {
		suhosin_log(S_VARS, "configured request variable name length limit exceeded - dropped variable '%s'", var);
		if (!SUHOSIN_G(simulation)) {
			return 0;
		}
	}
	if (SUHOSIN_G(max_totalname_length) && SUHOSIN_G(max_totalname_length) < total_len) {
		suhosin_log(S_VARS, "configured request variable total name length limit exceeded - dropped variable '%s'", var);
		if (!SUHOSIN_G(simulation)) {
			return 0;
		}
	}
	switch (arg) {
	    case PARSE_GET:
			if (SUHOSIN_G(max_get_name_length) && SUHOSIN_G(max_get_name_length) < var_len) {
				suhosin_log(S_VARS, "configured GET variable name length limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
					return 0;
				}
			}
			if (SUHOSIN_G(max_get_totalname_length) && SUHOSIN_G(max_get_totalname_length) < var_len) {
				suhosin_log(S_VARS, "configured GET variable total name length limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
					return 0;
				}
			}
			break;
	    case PARSE_COOKIE:
			if (SUHOSIN_G(max_cookie_name_length) && SUHOSIN_G(max_cookie_name_length) < var_len) {
				suhosin_log(S_VARS, "configured COOKIE variable name length limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
					return 0;
				}
			}
			if (SUHOSIN_G(max_cookie_totalname_length) && SUHOSIN_G(max_cookie_totalname_length) < var_len) {
				suhosin_log(S_VARS, "configured COOKIE variable total name length limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
					return 0;
				}
			}
			break;
	    case PARSE_POST:
			if (SUHOSIN_G(max_post_name_length) && SUHOSIN_G(max_post_name_length) < var_len) {
				suhosin_log(S_VARS, "configured POST variable name length limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
					return 0;
				}
			}
			if (SUHOSIN_G(max_post_totalname_length) && SUHOSIN_G(max_post_totalname_length) < var_len) {
				suhosin_log(S_VARS, "configured POST variable total name length limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
					return 0;
				}
			}
			break;
	}
	
	/* Find out array depth */
	while (index) {
		unsigned int index_length;
		
		depth++;
		index = strchr(index+1, '[');
		
		if (prev_index) {
			index_length = index ? index - 1 - prev_index - 1: strlen(prev_index);
			
			if (SUHOSIN_G(max_array_index_length) && SUHOSIN_G(max_array_index_length) < index_length) {
				suhosin_log(S_VARS, "configured request variable array index length limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
					return 0;
				}
			} 
			switch (arg) {
			    case PARSE_GET:
					if (SUHOSIN_G(max_get_array_index_length) && SUHOSIN_G(max_get_array_index_length) < index_length) {
						suhosin_log(S_VARS, "configured GET variable array index length limit exceeded - dropped variable '%s'", var);
						if (!SUHOSIN_G(simulation)) {
							return 0;
						}
					} 
					break;
			    case PARSE_COOKIE:
					if (SUHOSIN_G(max_cookie_array_index_length) && SUHOSIN_G(max_cookie_array_index_length) < index_length) {
						suhosin_log(S_VARS, "configured COOKIE variable array index length limit exceeded - dropped variable '%s'", var);
						if (!SUHOSIN_G(simulation)) {
							return 0;
						}
					} 
					break;
			    case PARSE_POST:
					if (SUHOSIN_G(max_post_array_index_length) && SUHOSIN_G(max_post_array_index_length) < index_length) {
						suhosin_log(S_VARS, "configured POST variable array index length limit exceeded - dropped variable '%s'", var);
						if (!SUHOSIN_G(simulation)) {
							return 0;
						}
					} 
					break;
			}
			prev_index = index;
		}
		
	}
	
	/* Drop this variable if it exceeds the array depth limit */
	if (SUHOSIN_G(max_array_depth) && SUHOSIN_G(max_array_depth) < depth) {
		suhosin_log(S_VARS, "configured request variable array depth limit exceeded - dropped variable '%s'", var);
		if (!SUHOSIN_G(simulation)) {
			return 0;
		}
	}
	switch (arg) {
	    case PARSE_GET:
			if (SUHOSIN_G(max_get_array_depth) && SUHOSIN_G(max_get_array_depth) < depth) {
				suhosin_log(S_VARS, "configured GET variable array depth limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
					return 0;
				}
			}
			break;
	    case PARSE_COOKIE:
			if (SUHOSIN_G(max_cookie_array_depth) && SUHOSIN_G(max_cookie_array_depth) < depth) {
				suhosin_log(S_VARS, "configured COOKIE variable array depth limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
					return 0;
				}
			}
			break;
	    case PARSE_POST:
			if (SUHOSIN_G(max_post_array_depth) && SUHOSIN_G(max_post_array_depth) < depth) {
				suhosin_log(S_VARS, "configured POST variable array depth limit exceeded - dropped variable '%s'", var);
				if (!SUHOSIN_G(simulation)) {
					return 0;
				}
			}
			break;
	}

	/* Check if variable value is truncated by a \0 */
	
	if (val && *val && val_len != strlen(*val)) {
	
		if (SUHOSIN_G(disallow_nul)) {
			suhosin_log(S_VARS, "ASCII-NUL chars not allowed within request variables - dropped variable '%s'", var);
			if (!SUHOSIN_G(simulation)) {
				return 0;
			}
		}
		switch (arg) {
		    case PARSE_GET:
				if (SUHOSIN_G(disallow_get_nul)) {
					suhosin_log(S_VARS, "ASCII-NUL chars not allowed within GET variables - dropped variable '%s'", var);
					if (!SUHOSIN_G(simulation)) {
						return 0;
					}
				}
				break;
		    case PARSE_COOKIE:
				if (SUHOSIN_G(disallow_cookie_nul)) {
					suhosin_log(S_VARS, "ASCII-NUL chars not allowed within COOKIE variables - dropped variable '%s'", var);
					if (!SUHOSIN_G(simulation)) {
						return 0;
					}
				}
				break;
		    case PARSE_POST:
				if (SUHOSIN_G(disallow_post_nul)) {
					suhosin_log(S_VARS, "ASCII-NUL chars not allowed within POST variables - dropped variable '%s'", var);
					if (!SUHOSIN_G(simulation)) {
						return 0;
					}
				}
				break;
		}
	}
	
	/* Drop this variable if it is one of GLOBALS, _GET, _POST, ... */
	/* This is to protect several silly scripts that do globalizing themself */
	
	switch (var_len) {
	    case 18:
		if (memcmp(var, "HTTP_RAW_POST_DATA", 18)==0) goto protected_varname;
		break;
	    case 17:
		if (memcmp(var, "HTTP_SESSION_VARS", 17)==0) goto protected_varname;
		break;
	    case 16:
		if (memcmp(var, "HTTP_SERVER_VARS", 16)==0) goto protected_varname;
		if (memcmp(var, "HTTP_COOKIE_VARS", 16)==0) goto protected_varname;
		break;
	    case 15:
		if (memcmp(var, "HTTP_POST_FILES", 15)==0) goto protected_varname;
		break;
	    case 14:
		if (memcmp(var, "HTTP_POST_VARS", 14)==0) goto protected_varname;
		break;
	    case 13:
		if (memcmp(var, "HTTP_GET_VARS", 13)==0) goto protected_varname;
		if (memcmp(var, "HTTP_ENV_VARS", 13)==0) goto protected_varname;
		break;
	    case 8:
		if (memcmp(var, "_SESSION", 8)==0) goto protected_varname;
		if (memcmp(var, "_REQUEST", 8)==0) goto protected_varname;
		break;
	    case 7:
		if (memcmp(var, "GLOBALS", 7)==0) goto protected_varname;
		if (memcmp(var, "_COOKIE", 7)==0) goto protected_varname;
		if (memcmp(var, "_SERVER", 7)==0) goto protected_varname;
		break;
	    case 6:
		if (memcmp(var, "_FILES", 6)==0) goto protected_varname;
		break;
	    case 5:
		if (memcmp(var, "_POST", 5)==0) goto protected_varname;
		break;
	    case 4:
		if (memcmp(var, "_ENV", 4)==0) goto protected_varname;
		if (memcmp(var, "_GET", 4)==0) goto protected_varname;
		break;
	}

	/* Okay let PHP register this variable */
	SUHOSIN_G(cur_request_variables)++;
	switch (arg) {
	    case PARSE_GET:
			SUHOSIN_G(cur_get_vars)++;
			break;
	    case PARSE_COOKIE:
			SUHOSIN_G(cur_cookie_vars)++;
			break;
	    case PARSE_POST:
			SUHOSIN_G(cur_post_vars)++;
			break;
	}
	
	if (new_val_len) {
		*new_val_len = val_len;
	}

	return 1;
protected_varname:
	suhosin_log(S_VARS, "tried to register forbidden variable '%s' through %s variables", var, arg == PARSE_GET ? "GET" : arg == PARSE_POST ? "POST" : "COOKIE");
	if (!SUHOSIN_G(simulation)) {
		return 0;
	} else {
		return 1;
	}
}
/* }}} */



/* {{{ suhosin_hook_register_server_variables
 */
void suhosin_hook_register_server_variables()
{
	orig_register_server_variables = sapi_module.register_server_variables;
	sapi_module.register_server_variables = suhosin_register_server_variables;
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */


