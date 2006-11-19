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
  $Id: crypt.c,v 1.2 2006-08-24 22:40:16 sesser Exp $ 
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_suhosin.h"


#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_CRYPT_H
#include <crypt.h>
#endif
#if TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef PHP_WIN32
#include <process.h>
extern char *crypt(char *__key, char *__salt);
#endif

#include "ext/standard/php_lcg.h"
#include "ext/standard/php_crypt.h"
#include "ext/standard/php_rand.h"


#define PHP_CRYPT_RAND php_rand(TSRMLS_C)

#define BLOWFISH_SALT_LEN 60

char *suhosin_crypt_blowfish_rn(char *key, char *setting, char *output, int size);
char *suhosin_crypt_gensalt_blowfish_rn(unsigned long count, char *input, int size, char *output, int output_size);

/*static unsigned char itoa64[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static void php_to64(char *s, long v, int n)
{
	while (--n >= 0) {
		*s++ = itoa64[v&0x3f]; 		
		v >>= 6;
	} 
}*/

/* {{{ proto string crypt(string str [, string salt])
   Encrypt a string */
static PHP_FUNCTION(suhosin_crypt)
{
	char salt[BLOWFISH_SALT_LEN+1];
	char *str, *salt_in = NULL;
	int str_len, salt_in_len;

	salt[0]=salt[BLOWFISH_SALT_LEN]='\0';
	/* This will produce suitable results if people depend on DES-encryption
	   available (passing always 2-character salt). At least for glibc6.1 */
	memset(&salt[1], '$', BLOWFISH_SALT_LEN-1);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &str, &str_len,
							  &salt_in, &salt_in_len) == FAILURE) {
		return;
	}

	if (salt_in) {
		memcpy(salt, salt_in, MIN(BLOWFISH_SALT_LEN, salt_in_len));
	}

	/* The automatic salt generation only covers standard DES and md5-crypt */
	if(!*salt) {
		char randat[16];
		int i;
		
		for (i=0; i<16; i++) randat[i] = PHP_CRYPT_RAND;
		
		suhosin_crypt_gensalt_blowfish_rn(5, randat, sizeof(randat), salt, sizeof(salt));
	}
	
	if (salt[0] == '$' &&
	    salt[1] == '2' &&
	    salt[2] == 'a' &&
	    salt[3] == '$' &&
	    salt[4] >= '0' && salt[4] <= '3' &&
	    salt[5] >= '0' && salt[5] <= '9' &&
	    salt[6] == '$') {
	    
		char output[BLOWFISH_SALT_LEN+1];
	    
		output[0] = 0;
		suhosin_crypt_blowfish_rn(str, salt, output, sizeof(output));
		RETVAL_STRING(output, 1);
	    
	} else {
		RETVAL_STRING(crypt(str, salt), 1);
	}
}
/* }}} */


/* {{{ suhosin_crypt_functions[]
 */
static function_entry suhosin_crypt_functions[] = {
	PHP_NAMED_FE(crypt, PHP_FN(suhosin_crypt), NULL)
	{NULL, NULL, NULL}
};
/* }}} */


void suhosin_hook_crypt()
{
	zend_constant *c;
	TSRMLS_FETCH();
	
	/* check if we already have blowfish support */
	if (zend_hash_find(EG(zend_constants), "CRYPT_BLOWFISH", sizeof("CRYPT_BLOWFISH"), (void **) &c) == FAILURE) {
		
		/* should we just bailout when this constant is not known at all? */
		return;		
	}
	
	if (c->value.type == IS_LONG && c->value.value.lval > 0) {
		
		/* blowfish support already installed */
		return;
	}
	
	/* mark blowfish as supported */
	c->value.type = IS_LONG;
	c->value.value.lval = 1;
	
	/* now fix the CRYPT_SALT_LENGTH constant */
	if (zend_hash_find(EG(zend_constants), "CRYPT_SALT_LENGTH", sizeof("CRYPT_SALT_LENGTH"), (void **) &c) == SUCCESS) {
		c->value.type = IS_LONG;
		c->value.value.lval = BLOWFISH_SALT_LEN;
	}
	
	/* replace the crypt() function */
	zend_hash_del(CG(function_table), "crypt", sizeof("crypt"));
#ifndef ZEND_ENGINE_2
	zend_register_functions(suhosin_crypt_functions, NULL, MODULE_PERSISTENT TSRMLS_CC);
#else
	zend_register_functions(NULL, suhosin_crypt_functions, NULL, MODULE_PERSISTENT TSRMLS_CC);
#endif
 
	
	
			
}


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */


