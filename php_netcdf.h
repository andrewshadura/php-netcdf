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

#ifndef PHP_NETCDF_H
#define PHP_NETCDF_H

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_NETCDF

#include <netcdf.h>
#include <php.h>
#include <php_ini.h>
#include <SAPI.h>
#include <ext/standard/info.h>
#include <Zend/zend_extensions.h>

extern zend_module_entry netcdf_module_entry;
#define phpext_netcdf_ptr &netcdf_module_entry


#ifdef PHP_WIN32
#define PHP_NETCDF_API __declspec(dllexport)
#else
#define PHP_NETCDF_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define FREE_RESOURCE(resource) zend_list_delete(Z_LVAL_P(resource))

#define PROP_GET_LONG(name)    Z_LVAL_P(zend_read_property(_this_ce, _this_zval, #name, strlen(#name), 1 TSRMLS_CC))
#define PROP_SET_LONG(name, l) zend_update_property_long(_this_ce, _this_zval, #name, strlen(#name), l TSRMLS_CC)

#define PROP_GET_DOUBLE(name)    Z_DVAL_P(zend_read_property(_this_ce, _this_zval, #name, strlen(#name), 1 TSRMLS_CC))
#define PROP_SET_DOUBLE(name, d) zend_update_property_double(_this_ce, _this_zval, #name, strlen(#name), d TSRMLS_CC)

#define PROP_GET_STRING(name)    Z_STRVAL_P(zend_read_property(_this_ce, _this_zval, #name, strlen(#name), 1 TSRMLS_CC))
#define PROP_GET_STRLEN(name)    Z_STRLEN_P(zend_read_property(_this_ce, _this_zval, #name, strlen(#name), 1 TSRMLS_CC))
#define PROP_SET_STRING(name, s) zend_update_property_string(_this_ce, _this_zval, #name, strlen(#name), s TSRMLS_CC)
#define PROP_SET_STRINGL(name, s, l) zend_update_property_stringl(_this_ce, _this_zval, #name, strlen(#name), s, l TSRMLS_CC)


PHP_MINIT_FUNCTION(netcdf);
PHP_MSHUTDOWN_FUNCTION(netcdf);
PHP_RINIT_FUNCTION(netcdf);
PHP_RSHUTDOWN_FUNCTION(netcdf);
PHP_MINFO_FUNCTION(netcdf);

PHP_METHOD(NetcdfDataset, __construct);
#if (PHP_MAJOR_VERSION >= 5)
ZEND_BEGIN_ARG_INFO_EX(NetcdfDataset____construct_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
  ZEND_ARG_INFO(0, mode)
  ZEND_ARG_INFO(0, clobber)
ZEND_END_ARG_INFO()
#else /* PHP 4.x */
#define NetcdfDataset____construct_args NULL
#endif

PHP_METHOD(NetcdfDataset, close);
#if (PHP_MAJOR_VERSION >= 5)
ZEND_BEGIN_ARG_INFO_EX(NetcdfDataset__close_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()
#else /* PHP 4.x */
#define NetcdfDataset__close_args NULL
#endif

PHP_METHOD(NetcdfDataset, sync);
#if (PHP_MAJOR_VERSION >= 5)
ZEND_BEGIN_ARG_INFO_EX(NetcdfDataset__sync_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()
#else /* PHP 4.x */
#define NetcdfDataset__sync_args NULL
#endif

PHP_METHOD(NetcdfDataset, _redef);
#if (PHP_MAJOR_VERSION >= 5)
ZEND_BEGIN_ARG_INFO_EX(NetcdfDataset___redef_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()
#else /* PHP 4.x */
#define NetcdfDataset___redef_args NULL
#endif

PHP_METHOD(NetcdfDataset, _enddef);
#if (PHP_MAJOR_VERSION >= 5)
ZEND_BEGIN_ARG_INFO_EX(NetcdfDataset___enddef_args, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()
#else /* PHP 4.x */
#define NetcdfDataset___enddef_args NULL
#endif

PHP_FUNCTION(nc_strerror);

PHP_FUNCTION(nc_inq_libvers);

PHP_FUNCTION(nc_strtype);

PHP_FUNCTION(nc_create);
PHP_FUNCTION(nc_open);
PHP_FUNCTION(nc_redef);
PHP_FUNCTION(nc_enddef);
PHP_FUNCTION(nc_close);

PHP_FUNCTION(nc_inq);
PHP_FUNCTION(nc_inq_ndims);
PHP_FUNCTION(nc_inq_nvars);
PHP_FUNCTION(nc_inq_natts);
PHP_FUNCTION(nc_inq_unlimdim);

PHP_FUNCTION(nc_sync);
PHP_FUNCTION(nc_abort);

PHP_FUNCTION(nc_set_fill);

PHP_FUNCTION(nc_def_dim);
PHP_FUNCTION(nc_inq_dimid);

PHP_FUNCTION(nc_inq_dim);
PHP_FUNCTION(nc_inq_dimname);
PHP_FUNCTION(nc_inq_dimlen);

PHP_FUNCTION(nc_rename_dim);

PHP_FUNCTION(nc_def_var);
PHP_FUNCTION(nc_inq_var);
PHP_FUNCTION(nc_inq_varid);
PHP_FUNCTION(nc_inq_varname);
PHP_FUNCTION(nc_inq_vartype);
PHP_FUNCTION(nc_inq_varndims);
PHP_FUNCTION(nc_inq_vardimid);
PHP_FUNCTION(nc_inq_varnatts);

PHP_FUNCTION(nc_inq_attname);

PHP_FUNCTION(nc_get_var1_float);

PHP_FUNCTION(nc_put_var_text);
PHP_FUNCTION(nc_put_var_uchar);
PHP_FUNCTION(nc_put_var_schar);
PHP_FUNCTION(nc_put_var_short);
PHP_FUNCTION(nc_put_var_int);
PHP_FUNCTION(nc_put_var_long);
PHP_FUNCTION(nc_put_var_float);
PHP_FUNCTION(nc_put_var_double);

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(netcdf)
	long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(netcdf)
*/

/* In every utility function you add that needs to use variables 
   in php_netcdf_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as NETCDF_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define NETCDF_G(v) TSRMG(netcdf_globals_id, zend_netcdf_globals *, v)
#else
#define NETCDF_G(v) (netcdf_globals.v)
#endif

#ifdef  __cplusplus
} // extern "C"
#endif

#endif /* HAVE_NETCDF */

#endif	/* PHP_NETCDF_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
