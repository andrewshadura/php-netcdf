/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2006 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Andrew O. Shadoura                                           |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_netcdf.h"

/* If you declare any globals in php_netcdf.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(netcdf)
*/

/* True global resources - no need for thread safety here */
static int le_netcdf;

/* {{{ netcdf_functions[]
 *
 * Every user visible function must have an entry in netcdf_functions[].
 */
zend_function_entry netcdf_functions[] = {
	PHP_FE(nc_strerror, NULL)

	PHP_FE(nc_inq_libvers, NULL)

	PHP_FE(nc_create, NULL)
	PHP_FE(nc_open, NULL)
	PHP_FE(nc_redef, NULL)
	PHP_FE(nc_enddef, NULL)
	PHP_FE(nc_close, NULL)

	PHP_FE(nc_inq, NULL)
	PHP_FE(nc_inq_ndims, NULL)
	PHP_FE(nc_inq_nvars, NULL)
	PHP_FE(nc_inq_natts, NULL)
	PHP_FE(nc_inq_unlimdim, NULL)

	PHP_FE(nc_sync, NULL)
	PHP_FE(nc_abort, NULL)

	PHP_FE(nc_set_fill, NULL)

	PHP_FE(nc_inq_dim, NULL)
	PHP_FE(nc_inq_dimname, NULL)
	PHP_FE(nc_inq_dimlen, NULL)
	PHP_FE(nc_inq_varname, NULL)
	PHP_FE(nc_inq_attname, NULL)

	{NULL, NULL, NULL}	/* Must be the last line in netcdf_functions[] */
};
/* }}} */

/* {{{ netcdf_module_entry
 */
zend_module_entry netcdf_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"netcdf",
	netcdf_functions,
	PHP_MINIT(netcdf),
	PHP_MSHUTDOWN(netcdf),
	PHP_RINIT(netcdf),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(netcdf),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(netcdf),
#if ZEND_MODULE_API_NO >= 20010901
	"0.0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_NETCDF
ZEND_GET_MODULE(netcdf)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("netcdf.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_netcdf_globals, netcdf_globals)
	STD_PHP_INI_ENTRY("netcdf.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_netcdf_globals, netcdf_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_netcdf_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_netcdf_init_globals(zend_netcdf_globals *netcdf_globals)
{
	netcdf_globals->global_value = 0;
	netcdf_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(netcdf)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(netcdf)
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
PHP_RINIT_FUNCTION(netcdf)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(netcdf)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(netcdf)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "netcdf support", "enabled");
	php_info_print_table_row(2, "Version", "0.0.1");
	php_info_print_table_row(2, "Revision", "$Rev$");
	php_info_print_table_row(2, "netCDF library version", nc_inq_libvers());
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


/* {{{ proto int nc_create(string path, int cmode, int &ncid)
   Creates a new netCDF dataset */
PHP_FUNCTION(nc_create)
{
	char *path = NULL;
	int path_len, cmode, ncid, result;
	zval *zncid;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slz", &path, &path_len, &cmode, &zncid) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	/* check for netCDF ID being passed by reference */
	if (!PZVAL_IS_REF(zncid))
	{
		zend_error(E_WARNING, "Variable for netCDF ID should be passed by reference");
		RETURN_NULL();
	}

	result=nc_create(path, cmode, &ncid);
	ZVAL_LONG(zncid, ncid);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_open(string path, int mode, int &ncid)
   Opens an existing netCDF dataset for access */
PHP_FUNCTION(nc_open)
{
	char *path = NULL;
	int path_len, mode, ncid, result;
	zval *zncid;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slz", &path, &path_len, &mode, &zncid) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	/* check for netCDF ID being passed by reference */
	if (!PZVAL_IS_REF(zncid))
	{
		zend_error(E_WARNING, "Variable for netCDF ID should be passed by reference");
		RETURN_NULL();
	}

	result=nc_open(path, mode, &ncid);
	ZVAL_LONG(zncid, ncid);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_redef(int ncid)
   Puts an open netCDF dataset into define mode */
PHP_FUNCTION(nc_redef)
{
	int ncid, result;

	if ((ZEND_NUM_ARGS() != 1) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ncid) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_redef(ncid);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_enddef(int ncid)
   Takes an open netCDF dataset out of define mode */
PHP_FUNCTION(nc_enddef)
{
	int ncid, result;

	if ((ZEND_NUM_ARGS() != 1) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ncid) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_enddef(ncid);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto string nc_close(int ncid)
   Closes an open netCDF dataset */
PHP_FUNCTION(nc_close)
{
	int ncid, result;

	if ((ZEND_NUM_ARGS() != 1) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ncid) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_close(ncid);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto string nc_inq(int ncid, int &ndims, int &nvars, int &ngatts, int &unlimdimid)
   Returns information about an open netCDF dataset */
PHP_FUNCTION(nc_inq)
{
	int ncid, ndims, nvars, ngatts, unlimdimid, result;
	zval *zndims, *znvars, *zngatts, *zunlimdimid;

	if ((ZEND_NUM_ARGS() != 5) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lzzzz", &ncid, &zndims, &znvars, &zngatts, &zunlimdimid) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid);
	ZVAL_LONG(zndims, ndims);
	ZVAL_LONG(znvars, nvars);
	ZVAL_LONG(zngatts, ngatts);
	ZVAL_LONG(zunlimdimid, unlimdimid);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_ndims(int ncid, int &ndims)
   Returns information about an open netCDF dataset */
PHP_FUNCTION(nc_inq_ndims)
{
	int ncid, ndims, result;
	zval *zndims;

	if ((ZEND_NUM_ARGS() != 2) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz", &ncid, &zndims) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_inq_ndims(ncid, &ndims);
	ZVAL_LONG(zndims, ndims);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_nvars(int ncid, int &nvars)
   Returns information about an open netCDF dataset */
PHP_FUNCTION(nc_inq_nvars)
{
	int ncid, nvars, result;
	zval *znvars;

	if ((ZEND_NUM_ARGS() != 2) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz", &ncid, &znvars) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_inq_nvars(ncid, &nvars);
	ZVAL_LONG(znvars, nvars);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_natts(int ncid, int &ngatts)
   Returns information about an open netCDF dataset */
PHP_FUNCTION(nc_inq_natts)
{
	int ncid, ngatts, result;
	zval *zngatts;

	if ((ZEND_NUM_ARGS() != 2) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz", &ncid, &zngatts) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_inq_natts(ncid, &ngatts);
	ZVAL_LONG(zngatts, ngatts);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_unlimdim(int ncid, int &unlimdimid)
   Returns information about an open netCDF dataset */
PHP_FUNCTION(nc_inq_unlimdim)
{
	int ncid, unlimdimid, result;
	zval *zunlimdimid;

	if ((ZEND_NUM_ARGS() != 2) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz", &ncid, &zunlimdimid) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_inq_unlimdim(ncid, &unlimdimid);
	ZVAL_LONG(zunlimdimid, unlimdimid);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_sync(int ncid)
   Synchronizes the disk copy of a netCDF dataset with in-memory buffers */
PHP_FUNCTION(nc_sync)
{
	int ncid, result;

	if ((ZEND_NUM_ARGS() != 1) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ncid) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_sync(ncid);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_abort(int ncid)
   Backs out of recent definitions */
PHP_FUNCTION(nc_abort)
{
	int ncid, result;

	if ((ZEND_NUM_ARGS() != 1) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ncid) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_abort(ncid);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_set_fill(int ncid, int fillmode, int &old_mode)
   Sets the fill mode for a netCDF dataset open for writing */
PHP_FUNCTION(nc_set_fill)
{
	int ncid, fillmode, old_mode, result;
	zval *zold_mode;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz", &ncid, &fillmode, &zold_mode) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_set_fill(ncid, fillmode, &old_mode);
	ZVAL_LONG(zold_mode, old_mode);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_dim(int ncid, int dimid, string &name, int &length)
   Returns information about an open netCDF dimension */

PHP_FUNCTION(nc_inq_dim)
{
	int ncid, dimid, length, result;
	char name[NC_MAX_NAME];
	zval *zname, *zlength;

	if ((ZEND_NUM_ARGS() != 4) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llzz", &ncid, &dimid, &zname, &zlength) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_inq_dim(ncid, dimid, name, &length);
	ZVAL_STRING(zname, name, 1);
	ZVAL_LONG(zlength, length);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_dimname(int ncid, int dimid, string &name)
   Returns information about an open netCDF dimension */

PHP_FUNCTION(nc_inq_dimname)
{
	int ncid, dimid, result;
	char name[NC_MAX_NAME];
	zval *zname;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz", &ncid, &dimid, &zname) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_inq_dimname(ncid, dimid, name);
	ZVAL_STRING(zname, name, 1);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_dimlen(int ncid, int dimid, int &length)
   Returns information about an open netCDF dimension */

PHP_FUNCTION(nc_inq_dimlen)
{
	int ncid, dimid, length, result;
	zval *zlength;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz", &ncid, &dimid, &zlength) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_inq_dimlen(ncid, dimid, &length);
	ZVAL_LONG(zlength, length);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_varname(int ncid, int varid, string &name)
   Returns information about a netCDF variable */

PHP_FUNCTION(nc_inq_varname)
{
	int ncid, varid, result;
	char name[NC_MAX_NAME];
	zval *zname;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz", &ncid, &varid, &zname) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_inq_varname(ncid, varid, name);
	ZVAL_STRING(zname, name, 1);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_attname(int ncid, int varid, int attnum, string &name)
   Returns information about a netCDF variable */

PHP_FUNCTION(nc_inq_attname)
{
	int ncid, varid, attnum, result;
	char name[NC_MAX_NAME];
	zval *zname;

	if ((ZEND_NUM_ARGS() != 4) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lllz", &ncid, &varid, &attnum, &zname) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_inq_attname(ncid, varid, attnum, name);
	ZVAL_STRING(zname, name, 1);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto string nc_strerror(int ncerr)
   Returns an error message string corresponding to an integer netCDF error status */

PHP_FUNCTION(nc_strerror)
{
	int ncerr;
	char* result;

	if ((ZEND_NUM_ARGS() != 1) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ncerr) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_strerror(ncerr);

	RETURN_STRING(result, 1);
}
/* }}} */

/* {{{ proto string nc_nc_inq_libvers()
   Returns a string identifying the version of the netCDF library, and when it was built */

PHP_FUNCTION(nc_inq_libvers)
{
	char* result;
	zval *zlibvers;

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	result=nc_inq_libvers();

	RETURN_STRING(result, 1);
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
