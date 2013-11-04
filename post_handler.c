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
  $Id: post_handler.c,v 1.10 2007-03-04 17:54:05 sesser Exp $ 
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_suhosin.h"
#include "SAPI.h"
#include "php_variables.h"
#include "php_content_types.h"
#include "suhosin_rfc1867.h"
#include "ext/standard/url.h"

SAPI_POST_HANDLER_FUNC(suhosin_rfc1867_post_handler);


SAPI_POST_HANDLER_FUNC(suhosin_std_post_handler)
{
	char *var, *val;
	char *strtok_buf = NULL;
	zval *array_ptr = (zval *) arg;

	if (SG(request_info).post_data == NULL) {
		return;
	}	

	var = php_strtok_r(SG(request_info).post_data, "&", &strtok_buf);

	while (var) {
		val = strchr(var, '=');
		if (val) { /* have a value */
			unsigned int val_len, new_val_len;

			*val++ = '\0';
			php_url_decode(var, strlen(var));
			val_len = php_url_decode(val, strlen(val));
			val = estrndup(val, val_len);
			if (suhosin_input_filter(PARSE_POST, var, &val, val_len, &new_val_len TSRMLS_CC)) {
#ifdef ZEND_ENGINE_2
				if (sapi_module.input_filter(PARSE_POST, var, &val, new_val_len, &new_val_len TSRMLS_CC)) {
#endif
					php_register_variable_safe(var, val, new_val_len, array_ptr TSRMLS_CC);
#ifdef ZEND_ENGINE_2
				}
#endif
			} else {
				SUHOSIN_G(abort_request)=1;
			}
			efree(val);
		}
		var = php_strtok_r(NULL, "&", &strtok_buf);
	}
}

/* {{{ php_post_entries[]
 */
static sapi_post_entry suhosin_post_entries[] = {
	{ DEFAULT_POST_CONTENT_TYPE, sizeof(DEFAULT_POST_CONTENT_TYPE)-1, sapi_read_standard_form_data,	suhosin_std_post_handler },
	{ MULTIPART_CONTENT_TYPE,    sizeof(MULTIPART_CONTENT_TYPE)-1,    NULL,                         suhosin_rfc1867_post_handler },
	{ NULL, 0, NULL, NULL }
};
/* }}} */

void suhosin_hook_post_handlers(TSRMLS_D)
{
#if PHP_MAJOR_VERSION > 5 || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION > 0)
	sapi_unregister_post_entry(&suhosin_post_entries[0] TSRMLS_CC);
	sapi_unregister_post_entry(&suhosin_post_entries[1] TSRMLS_CC);
	sapi_register_post_entries(suhosin_post_entries TSRMLS_CC);
#else
	sapi_unregister_post_entry(&suhosin_post_entries[0]);
	sapi_unregister_post_entry(&suhosin_post_entries[1]);
	sapi_register_post_entries(suhosin_post_entries);
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


