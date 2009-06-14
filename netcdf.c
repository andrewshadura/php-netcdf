/*
    This file is part of netCDF for PHP.

    Copyright (C) 2007-2009 Andrew O. Shadoura

This package is open source software; you can redistribute it and/or modify                                                                    
it under the terms of either:                                                                                                                  

    netCDF for PHP is free software; you can redistribute it and/or modify
    it under the terms of either:
      - the GNU General Public License as published by
        the Free Software Foundation, version 2 of the License, or
      - the GNU Lesser General Public License as published by
        the Free Software Foundation, version 2.1 of the License

    netCDF for PHP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    License for more details.

    You should have received a copy of the GNU General Public License
    and GNU Lesser General Public License along with netCDF for PHP.
    If not, see <http://www.gnu.org/licenses/>.
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

static char* netcdf_types[7] = { "NC_NAT", "NC_BYTE", "NC_CHAR", "NC_SHORT", "NC_INT", "NC_FLOAT", "NC_DOUBLE" };

/* {{{ netcdf_functions[]
 *
 * Every user visible function must have an entry in netcdf_functions[].
 */
zend_function_entry netcdf_functions[] = {
	PHP_FE(nc_strerror, NULL)

	PHP_FE(nc_inq_libvers, NULL)

	PHP_FE(nc_strtype, NULL)

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

	PHP_FE(nc_def_dim, NULL)
	PHP_FE(nc_inq_dimid, NULL)

	PHP_FE(nc_inq_dim, NULL)
	PHP_FE(nc_inq_dimname, NULL)
	PHP_FE(nc_inq_dimlen, NULL)

	PHP_FE(nc_rename_dim, NULL)

	PHP_FE(nc_def_var, NULL)
	PHP_FE(nc_inq_var, NULL)
	PHP_FE(nc_inq_varname, NULL)
	PHP_FE(nc_inq_vartype, NULL)
	PHP_FE(nc_inq_varndims, NULL)
	PHP_FE(nc_inq_vardimid, NULL)
	PHP_FE(nc_inq_varnatts, NULL)
	PHP_FE(nc_inq_attname, NULL)

	PHP_FE(nc_put_var_text,NULL)
	PHP_FE(nc_put_var_uchar,NULL)
	PHP_FE(nc_put_var_schar,NULL)
	PHP_FE(nc_put_var_short,NULL)
	PHP_FE(nc_put_var_int,NULL)
	PHP_FE(nc_put_var_long,NULL)
	PHP_FE(nc_put_var_float,NULL)
	PHP_FE(nc_put_var_double,NULL)

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
	REGISTER_LONG_CONSTANT("NC_NAT", NC_NAT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_BYTE", NC_BYTE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_CHAR", NC_CHAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_SHORT", NC_SHORT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_INT", NC_INT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_FLOAT", NC_FLOAT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_DOUBLE", NC_DOUBLE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_FILL_BYTE", NC_FILL_BYTE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_FILL_CHAR", NC_FILL_CHAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_FILL_SHORT", NC_FILL_SHORT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_FILL_INT", NC_FILL_INT, CONST_CS | CONST_PERSISTENT);
	REGISTER_DOUBLE_CONSTANT("NC_FILL_FLOAT", NC_FILL_FLOAT, CONST_CS | CONST_PERSISTENT);
	REGISTER_DOUBLE_CONSTANT("NC_FILL_DOUBLE", NC_FILL_DOUBLE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_NOWRITE", NC_NOWRITE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_WRITE", NC_WRITE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_CLOBBER", NC_CLOBBER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_NOCLOBBER", NC_NOCLOBBER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_FILL", NC_FILL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_NOFILL", NC_NOFILL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_LOCK", NC_LOCK, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_SHARE", NC_SHARE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_SIZEHINT_DEFAULT", NC_SIZEHINT_DEFAULT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_ALIGN_CHUNK", NC_ALIGN_CHUNK, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_UNLIMITED", NC_UNLIMITED, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_GLOBAL", NC_GLOBAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_MAX_DIMS", NC_MAX_DIMS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_MAX_ATTRS", NC_MAX_ATTRS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_MAX_VARS", NC_MAX_VARS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_MAX_NAME", NC_MAX_NAME, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_MAX_VAR_DIMS", NC_MAX_VAR_DIMS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_ENOTATT", NC_ENOTATT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_EBADTYPE", NC_EBADTYPE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_EBADDIM", NC_EBADDIM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_EUNLIMPOS", NC_EUNLIMPOS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_ENOTVAR", NC_ENOTVAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_EGLOBAL", NC_EGLOBAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_ENOTNC", NC_ENOTNC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_ESTS", NC_ESTS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_EMAXNAME", NC_EMAXNAME, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_EUNLIMIT", NC_EUNLIMIT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_ENORECVARS", NC_ENORECVARS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_ECHAR", NC_ECHAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_EEDGE", NC_EEDGE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_ESTRIDE", NC_ESTRIDE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_EBADNAME", NC_EBADNAME, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_ERANGE", NC_ERANGE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_ENOMEM", NC_ENOMEM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_LONG", NC_LONG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("NC_ENTOOL", NC_ENTOOL, CONST_CS | CONST_PERSISTENT);
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
	long path_len, cmode, result;
	int ncid;
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
	long path_len, mode, result;
	int ncid;
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
	long ncid, result;

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
	long ncid, result;

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
	long ncid, result;

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
	long ncid, result;
	int ndims, nvars, ngatts, unlimdimid;
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
	long ncid, result;
	int ndims;
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
	long ncid, result;
	int nvars;
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
	long ncid, result;
	int ngatts;
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
	long ncid, result;
	int unlimdimid;
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
	long ncid, result;

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
	long ncid, result;

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
	long ncid, fillmode, result;
	int old_mode;
	zval *zold_mode;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz", &ncid, &fillmode, &zold_mode) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_set_fill(ncid, fillmode, &old_mode);
	ZVAL_LONG(zold_mode, old_mode);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_def_dim(int ncid, string name, int len, int &dimid)
   Adds a new dimension to an open netCDF dataset in define mode */
PHP_FUNCTION(nc_def_dim)
{
	long ncid, len, namelen, result;
	int dimid;
	char *name = NULL;
	zval *zdimid;

	if ((ZEND_NUM_ARGS() != 4) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lslz", &ncid, &name, &namelen, &len, &zdimid) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_def_dim(ncid, name, len, &dimid);
	ZVAL_LONG(zdimid, dimid);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_dimid(int ncid, string name, int &dimid)
   Returns the ID of a netCDF dimension, given the name of the dimension */
PHP_FUNCTION(nc_inq_dimid)
{
	long ncid, namelen, result;
	int dimid;
	char *name = NULL;
	zval *zdimid;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsz", &ncid, &name, &namelen, &zdimid) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_inq_dimid(ncid, name, &dimid);
	ZVAL_LONG(zdimid, dimid);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_dim(int ncid, int dimid, string &name, int &length)
   Returns information about an open netCDF dimension */

PHP_FUNCTION(nc_inq_dim)
{
	long ncid, dimid, result;
	int length;
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
	long ncid, dimid, result;
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
	long ncid, dimid, result;
	int length;
	zval *zlength;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz", &ncid, &dimid, &zlength) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_inq_dimlen(ncid, dimid, &length);
	ZVAL_LONG(zlength, length);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_rename_dim(int ncid, int dimid, string name)
   Renames an existing dimension in a netCDF dataset open for writing */
PHP_FUNCTION(nc_rename_dim)
{
	long ncid, dimid, namelen, result;
	char *name = NULL;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lls", &ncid, &dimid, &name, &namelen) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	result=nc_rename_dim(ncid, dimid, name);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_def_var(int ncid, string name, int xtype, int ndims, int dimids[], int &varid)
   Adds a new variable to an open netCDF dataset in define mode */

PHP_FUNCTION(nc_def_var)
{
	long ncid, xtype, ndims, i=0;
	int varid;
	char *name = NULL;
	long namelen;
	int dimids[NC_MAX_VAR_DIMS];
	long result;
	zval *zdimids, *zvarid;
	zval **data;
	HashTable *arr_hash;
	HashPosition pointer;

	if ((ZEND_NUM_ARGS() != 6) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsllaz", &ncid, &name, &namelen, &xtype, &ndims, &zdimids, &zvarid) != SUCCESS)) {
		WRONG_PARAM_COUNT;
	}

	arr_hash = Z_ARRVAL_P(zdimids);
	for (zend_hash_internal_pointer_reset_ex(arr_hash, &pointer); (i<ndims) && (zend_hash_get_current_data_ex(arr_hash, (void**) &data, &pointer) == SUCCESS); i++)
	{
		if (Z_TYPE_PP(data) == IS_LONG)
		{
			dimids[i]=Z_LVAL_PP(data);
		}
		zend_hash_move_forward_ex(arr_hash, &pointer);
	}
	result=nc_def_var(ncid, name, xtype, ndims, dimids, &varid);
	ZVAL_LONG(zvarid, varid);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_var(int ncid, int varid, string &name,
 int *xtypep, int *ndimsp, int *dimidsp, int *nattsp); */
PHP_FUNCTION(nc_inq_var)
{
	int argc = ZEND_NUM_ARGS();
	long ncid;
	long varid;
	zval *zvarid;

	php_error(E_WARNING, "nc_inq_var: not yet implemented");

	if (zend_parse_parameters(argc TSRMLS_CC, "llz", &ncid, &varid) == FAILURE) 
		WRONG_PARAM_COUNT;
}
/* }}} */

/* {{{ proto int nc_inq_varid(int ncid, string name, int &varid)
   Returns the ID of a netCDF variable, given its name */

PHP_FUNCTION(nc_inq_varid)
{
	int ncid, varid, namelen, result;
	char *name = NULL;
	zval *zvarid;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsz", &ncid, &name, &namelen, &zvarid) != SUCCESS))
		WRONG_PARAM_COUNT;

	result=nc_inq_varid(ncid, name, &varid);
	ZVAL_LONG(zvarid, varid);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_varname(int ncid, int varid, string &name)
   Returns information about a netCDF variable */

PHP_FUNCTION(nc_inq_varname)
{
	long ncid, varid, result;
	char name[NC_MAX_NAME];
	zval *zname;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz", &ncid, &varid, &zname) != SUCCESS))
		WRONG_PARAM_COUNT;

	result=nc_inq_varname(ncid, varid, name);
	ZVAL_STRING(zname, name, 1);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_vartype(int ncid, int varid, int &xtype)
   Returns information about a netCDF variable */
PHP_FUNCTION(nc_inq_vartype)
{
	long ncid, varid, result;
	nc_type xtype;
	zval *zxtype;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz", &ncid, &varid, &zxtype) != SUCCESS))
		WRONG_PARAM_COUNT;

	result=nc_inq_vartype(ncid, varid, &xtype);
	ZVAL_LONG(zxtype, xtype);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_varndims(int ncid, int varid, int &ndimsp)
   Returns information about a netCDF variable */
PHP_FUNCTION(nc_inq_varndims)
{
	long ncid, varid, result;
	int ndims;
	zval *zndims;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz", &ncid, &varid, &zndims) != SUCCESS))
		WRONG_PARAM_COUNT;

	result=nc_inq_varndims(ncid, varid, &ndims);
	ZVAL_LONG(zndims, ndims);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_vardimid(int ncid, int varid, int )
   * dimidsp); */
PHP_FUNCTION(nc_inq_vardimid)
{
	int argc = ZEND_NUM_ARGS();
	long ncid;
	long varid;
	long dimids;

	php_error(E_WARNING, "nc_inq_vardimid: not yet implemented");

	if (zend_parse_parameters(argc TSRMLS_CC, "lll", &ncid, &varid, &dimids) == FAILURE) 
		WRONG_PARAM_COUNT;
}
/* }}} */

/* {{{ proto int nc_inq_varnatts(int ncid, int varid, int )
   * nattsp); */
PHP_FUNCTION(nc_inq_varnatts)
{
	int argc = ZEND_NUM_ARGS();
	long ncid;
	long varid;
	long natts;

	php_error(E_WARNING, "nc_inq_varnatts: not yet implemented");

	if (zend_parse_parameters(argc TSRMLS_CC, "lll", &ncid, &varid, &natts) == FAILURE) 
		WRONG_PARAM_COUNT;
}
/* }}} */

/* {{{ proto int nc_inq_attname(int ncid, int varid, int attnum, string &name)
   Returns information about a netCDF variable */

PHP_FUNCTION(nc_inq_attname)
{
	long ncid, varid, attnum, result;
	char name[NC_MAX_NAME];
	zval *zname;

	if ((ZEND_NUM_ARGS() != 4) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lllz", &ncid, &varid, &attnum, &zname) != SUCCESS))
		WRONG_PARAM_COUNT;

	result=nc_inq_attname(ncid, varid, attnum, name);
	ZVAL_STRING(zname, name, 1);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_get_var1_float(int ncid, int varid)
   unsigned int * indexp, float * ip); */
PHP_FUNCTION(nc_get_var1_float)
{
	int argc = ZEND_NUM_ARGS();
	long ncid;
	long varid;

	if (zend_parse_parameters(argc TSRMLS_CC, "ll", &ncid, &varid) == FAILURE) 
		return;

	php_error(E_WARNING, "nc_get_var1_float: not yet implemented");
}
/* }}} */

/* {{{ proto int nc_get_var_float(int ncid, int varid, double &f); */
PHP_FUNCTION(nc_get_var_float)
{
	int argc = ZEND_NUM_ARGS();
	long ncid;
	long varid;
	zval *zf;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz", &ncid, &varid, &zf) != SUCCESS))
		return;

	//array_init(zf);
	php_error(E_WARNING, "nc_get_var_float: not yet implemented");
}
/* }}} */

/* {{{ proto int nc_put_var_text(int ncid, int varid, string &blob); */
PHP_FUNCTION(nc_put_var_text)
{
	int argc = ZEND_NUM_ARGS();
	long ncid;
	long varid;
	long result;
	long bloblen;
	void *blob = NULL;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lls", &ncid, &varid, &blob, &bloblen) != SUCCESS))
		return;

	result = nc_put_var_text(ncid, varid, blob);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_put_var_uchar(int ncid, int varid, string &blob, &bloblen); */
PHP_FUNCTION(nc_put_var_uchar)
{
	int argc = ZEND_NUM_ARGS();
	long ncid;
	long varid;
	long result;
	long bloblen;
	void *blob = NULL;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lls", &ncid, &varid, &blob, &bloblen) != SUCCESS))
		return;

	result = nc_put_var_uchar(ncid, varid, blob);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_put_var_schar(int ncid, int varid, string &blob, &bloblen); */
PHP_FUNCTION(nc_put_var_schar)
{
	int argc = ZEND_NUM_ARGS();
	long ncid;
	long varid;
	long result;
	long bloblen;
	void *blob = NULL;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lls", &ncid, &varid, &blob, &bloblen) != SUCCESS))
		return;

	result = nc_put_var_schar(ncid, varid, blob);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_put_var_short(int ncid, int varid, string &blob, &bloblen); */
PHP_FUNCTION(nc_put_var_short)
{
	int argc = ZEND_NUM_ARGS();
	long ncid;
	long varid;
	long result;
	long bloblen;
	void *blob = NULL;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lls", &ncid, &varid, &blob, &bloblen) != SUCCESS))
		return;

	result = nc_put_var_short(ncid, varid, blob);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_put_var_int(int ncid, int varid, string &blob, &bloblen); */
PHP_FUNCTION(nc_put_var_int)
{
	int argc = ZEND_NUM_ARGS();
	long ncid;
	long varid;
	long result;
	long bloblen;
	void *blob = NULL;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lls", &ncid, &varid, &blob, &bloblen) != SUCCESS))
		return;

	result = nc_put_var_int(ncid, varid, blob);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_put_var_long(int ncid, int varid, string &blob, &bloblen); */
PHP_FUNCTION(nc_put_var_long)
{
	int argc = ZEND_NUM_ARGS();
	long ncid;
	long varid;
	long result;
	long bloblen;
	void *blob = NULL;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lls", &ncid, &varid, &blob, &bloblen) != SUCCESS))
		return;

	result = nc_put_var_long(ncid, varid, blob);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_put_var_float(int ncid, int varid, string &blob, &bloblen); */
PHP_FUNCTION(nc_put_var_float)
{
	int argc = ZEND_NUM_ARGS();
	long ncid;
	long varid;
	long result;
	long bloblen;
	void *blob = NULL;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lls", &ncid, &varid, &blob, &bloblen) != SUCCESS))
		return;

	result = nc_put_var_float(ncid, varid, blob);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_put_var_double(int ncid, int varid, string &blob, &bloblen); */
PHP_FUNCTION(nc_put_var_double)
{
	int argc = ZEND_NUM_ARGS();
	long ncid;
	long varid;
	long result;
	long bloblen;
	void *blob = NULL;

	if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lls", &ncid, &varid, &blob, &bloblen) != SUCCESS))
		return;

	result = nc_put_var_double(ncid, varid, blob);

	RETURN_LONG(result);
}
/* }}} */

/* {{{ proto string nc_strerror(int ncerr)
   Returns an error message string corresponding to an integer netCDF error status */

PHP_FUNCTION(nc_strerror)
{
	long ncerr;
	char* result;

	if ((ZEND_NUM_ARGS() != 1) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ncerr) != SUCCESS))
		WRONG_PARAM_COUNT;

	result=(char *) nc_strerror(ncerr);

	RETURN_STRING(result, 1);
}
/* }}} */

/* {{{ proto string nc_inq_libvers()
   Returns a string identifying the version of the netCDF library, and when it was built */

PHP_FUNCTION(nc_inq_libvers)
{
	char* result;
	zval *zlibvers;

	if (ZEND_NUM_ARGS())
		WRONG_PARAM_COUNT;

	result=(char *) nc_inq_libvers();

	RETURN_STRING(result, 1);
}
/* }}} */

/* {{{ proto string nc_strtype(int xtype)
   Returns a string name of netCDF type by its ID */

PHP_FUNCTION(nc_strtype)
{
	char* result;
	long xtype;
	zval *zlibvers;

	if ((ZEND_NUM_ARGS() != 1) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &xtype) != SUCCESS))
		WRONG_PARAM_COUNT;


	result=(char *) netcdf_types[xtype];

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
