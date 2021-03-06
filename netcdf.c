/*
    This file is part of netCDF for PHP.

    Copyright (C) 2007-2010 Andrej Shadura
    Copyright (C)      2010 Santi Oliveras

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

#include "php_netcdf.h"

/* If you declare any globals in php_netcdf.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(netcdf)
*/

/* True global resources - no need for thread safety here */
static int le_netcdf;

static char* netcdf_types[7] = { "NC_NAT", "NC_BYTE", "NC_CHAR", "NC_SHORT", "NC_INT", "NC_FLOAT", "NC_DOUBLE" };

static zend_class_entry * NetcdfDataset_entry_ptr = NULL;
#define PHP_NETCDF_DATASET_NAME "NetcdfDataset"

#define ISSUE_WARNING(msg) { zend_error(E_WARNING, "%s: %s", __func__, msg); }
#define RETURN_ERROR(msg) { zend_error(E_WARNING, "%s: %s", __func__, msg); RETURN_NULL(); }
#define RETURN_NETCDF_ERROR(msg,error) { zend_error(E_WARNING, "%s: %s\n         %s", __func__, msg, nc_strerror(error)); RETURN_NULL(); }


/* utility functions */

void *allocate_space(nc_type at_type, size_t at_len) {
    switch(at_type) {
        case NC_BYTE:
            return emalloc(at_len * sizeof (char));
        case NC_CHAR:
            return emalloc(at_len * sizeof (char) + 1); /* + 1 for trailing null */
        case NC_SHORT:
            return emalloc(at_len * sizeof (short));
        case NC_INT:
        case NC_FLOAT:
            return emalloc(at_len * sizeof (int));
        case NC_DOUBLE:
            return emalloc(at_len * sizeof (double));
    }
    ISSUE_WARNING("Unsupported netCDF type");
    return NULL;
}

void assign_zval(nc_type at_type, size_t at_len, void *value, zval *zvalue) {
    int i;
    char *chars;

    if (at_type == NC_CHAR) {
        chars = (char *)value;
        chars[at_len] = 0;
        ZVAL_STRING(zvalue, chars);
        return;
    }

    if (at_len == 1) {
        switch(at_type) {
            case NC_BYTE:
                ZVAL_LONG(zvalue, *(char *)value);
                break;
            case NC_SHORT:
                ZVAL_LONG(zvalue, *(short *)value);
                break;
            case NC_INT:
                ZVAL_LONG(zvalue, *(int *)value);
                break;
            case NC_FLOAT:
                ZVAL_DOUBLE(zvalue,*(float *)value);
                break;
            case NC_DOUBLE:
                ZVAL_DOUBLE(zvalue,*(double *)value);
        }
        return;
    }

    array_init(zvalue);
    for(i = 0; i < at_len; i++)
    {
        switch(at_type)
        {
            case NC_BYTE:
                add_index_long(zvalue, i, *(((char *)value) + i));
                break;
            case NC_SHORT:
                add_index_long(zvalue, i, *(((short *)value) + i));
                break;
            case NC_INT:
                add_index_long(zvalue, i, *(((int *)value) + i));
                break;
            case NC_FLOAT:
                add_index_double(zvalue, i, *(((float *)value) + i));
                break;
            case NC_DOUBLE:
                add_index_double(zvalue, i, *(((double *)value) + i));
        }
    }
}

int assign_value(nc_type at_type, size_t at_len, zval *zvalue, void *value){
    int i;
    zval *data;
    HashTable *arr_hash;
    HashPosition pointer;

    if(at_type == NC_CHAR)
    {
        snprintf((char *) value, Z_STRLEN_P(zvalue) + 1, "%s", Z_STRVAL_P(zvalue));
        return 1;
    }

    if(at_len == 1)
    {
        switch(at_type){
            case NC_BYTE:
                *(char *)value = Z_LVAL_P(zvalue);
                break;
            case NC_SHORT:
                *(short *)value = Z_LVAL_P(zvalue);
                break;
            case NC_INT:
                *(int *)value = Z_LVAL_P(zvalue);
                break;
            case NC_FLOAT:
                *(float *)value = Z_DVAL_P(zvalue);
                break;
            case NC_DOUBLE:
                *(double *)value = Z_DVAL_P(zvalue);
        }
        return 1;
    }

    arr_hash = Z_ARRVAL_P(zvalue);
    if(at_len != zend_hash_num_elements(arr_hash))
    {
        ISSUE_WARNING("The number of elements in array must correspond to the variable's dimensions");
        return 0;
    }

    for (zend_hash_internal_pointer_reset_ex(arr_hash, &pointer), i = 0;
        (i < at_len) && ((data = zend_hash_get_current_data_ex(arr_hash, &pointer)) != NULL);
        zend_hash_move_forward_ex(arr_hash, &pointer), i++)
    {
       switch(at_type)
       {
            case NC_BYTE:
                *(((char *)value) + i) = Z_LVAL_P(data);
                break;
            case NC_SHORT:
                *(((short *)value) + i) = Z_LVAL_P(data);
                break;
            case NC_INT:
                *(((int *)value) + i) = Z_LVAL_P(data);
                break;
            case NC_FLOAT:
                *(((float *)value) + i) = Z_DVAL_P(data);
                break;
            case NC_DOUBLE:
                *(((double *)value) + i) = Z_DVAL_P(data);
        }
    }
    return 1;
}

void *get_values_from_array(nc_type at_type, zval *zvalue){
    uint i;
    zval *data;
    HashTable *arr_hash;
    HashPosition pointer;
    void *values;

    arr_hash = HASH_OF(zvalue);

    values = allocate_space(at_type, zend_hash_num_elements(arr_hash));
    if (values == NULL)
    {
        ISSUE_WARNING("error allocating memory");
        return NULL;
    }

    for (zend_hash_internal_pointer_reset_ex(arr_hash, &pointer), i = 0;
        (data = zend_hash_get_current_data_ex(arr_hash, &pointer)) != NULL;
        zend_hash_move_forward_ex(arr_hash, &pointer), i++)
    {
       switch(at_type)
       {
            case NC_BYTE:
                *(((char *)values) + i) = Z_LVAL_P(data);
                break;
            case NC_SHORT:
                *(((short *)values) + i) = Z_LVAL_P(data);
                break;
            case NC_INT:
                *(((int *)values) + i) = Z_LVAL_P(data);
                break;
            case NC_FLOAT:
                *(((float *)values) + i) = Z_DVAL_P(data);
                break;
            case NC_DOUBLE:
                *(((double *)values) + i) = Z_DVAL_P(data);
        }
    }
    return values;
}

#ifndef CONFIG_NETCDF_4
/* Function inserted as NETCDF-3 doesn't implement it. */
int nc_get_var(int ncid, int varid, void *values) {
    int result;
    nc_type xtype;

    result = nc_inq_vartype(ncid, varid, &xtype);
    if (result != NC_NOERR) return result;
    switch (xtype){
        case NC_CHAR:
            return nc_get_var_text(ncid, varid, (char *)values);
        case NC_BYTE:
            return nc_get_var_schar(ncid, varid, (signed char *)values);
        case NC_SHORT:
            return nc_get_var_short(ncid, varid, (short *)values);
        case NC_INT:
            return nc_get_var_int(ncid, varid, (int *)values);
        case NC_FLOAT:
            return nc_get_var_float(ncid, varid, (float *)values);
        case NC_DOUBLE:
            return nc_get_var_double(ncid, varid, (double *)values);
    }
    return 2;
}

int nc_put_var(int ncid, int varid, const void *values)
{
    int result;
    nc_type xtype;

    result = nc_inq_vartype(ncid, varid, &xtype);
    if (result != NC_NOERR) return result;
    switch(xtype)
    {
        case NC_CHAR:
            return nc_put_var_text(ncid, varid, (char *) values);
        case NC_BYTE:
            return nc_put_var_schar(ncid, varid, (signed char *) values);
        case NC_SHORT:
            return nc_put_var_short(ncid, varid, (short *) values);
        case NC_INT:
            return nc_put_var_int(ncid, varid, (int *) values);
        case NC_FLOAT:
            return nc_put_var_float(ncid, varid, (float *) values);
        case NC_DOUBLE:
            return nc_put_var_double(ncid, varid, (double *) values);
    }
    return 2;
}
#endif

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_create, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, mode, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(1, ncid, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_open, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, mode, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(1, ncid, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_inq, 0, 0, 5)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(1, ndims, IS_LONG, 1)
    ZEND_ARG_TYPE_INFO(1, nvars, IS_LONG, 1)
    ZEND_ARG_TYPE_INFO(1, ngatts, IS_LONG, 1)
    ZEND_ARG_TYPE_INFO(1, unlimdimid, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_inq_ndims, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(1, ndims, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_inq_nvars, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(1, nvars, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_inq_natts, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(1, ngatts, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_inq_unlimdim, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(1, unlimdimid, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_set_fill, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, fillmode, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(1, old_mode, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_def_dim, 0, 0, 4)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, len, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(1, dimid, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_inq_dimid, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(1, dimid, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_inq_dim, 0, 0, 4)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, dimid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(1, name, IS_STRING, 1)
    ZEND_ARG_TYPE_INFO(1, length, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_inq_dimname, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, dimid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(1, name, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_inq_dimlen, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, dimid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(1, length, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_inq_var, 0, 0, 7)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, varid, IS_LONG, 1)
    ZEND_ARG_TYPE_INFO(1, name, IS_STRING, 1)
    ZEND_ARG_TYPE_INFO(1, xtypep, IS_LONG, 1)
    ZEND_ARG_TYPE_INFO(1, ndimsp, IS_LONG, 1)
    ZEND_ARG_INFO     (1, dimidsp)
    ZEND_ARG_TYPE_INFO(1, nattsp, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_inq_attname, 0, 0, 4)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, varid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, attnum, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(1, name, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_get_att, 0, 0, 4)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, varid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
    ZEND_ARG_INFO     (1, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_get_var1, 0, 0, 4)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, varid, IS_LONG, 0)
    ZEND_ARG_INFO     (0, indexp)
    ZEND_ARG_INFO     (1, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nc_get_vara, 0, 0, 4)
    ZEND_ARG_TYPE_INFO(0, ncid, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, varid, IS_LONG, 0)
    ZEND_ARG_INFO     (0, start)
    ZEND_ARG_INFO     (0, count)
    ZEND_ARG_INFO     (1, values)
ZEND_END_ARG_INFO()


/* {{{ netcdf_functions[]
 *
 * Every user visible function must have an entry in netcdf_functions[].
 */
zend_function_entry netcdf_functions[] = {
    PHP_FE(nc_strerror, NULL)

    PHP_FE(nc_inq_libvers, NULL)

    PHP_FE(nc_strtype, NULL)

    PHP_FE(nc_create, arginfo_nc_create)
    PHP_FE(nc_open, arginfo_nc_open)
    PHP_FE(nc_redef, NULL)
    PHP_FE(nc_enddef, NULL)
    PHP_FE(nc_close, NULL)

    PHP_FE(nc_inq, arginfo_nc_inq)
    PHP_FE(nc_inq_ndims, arginfo_nc_inq_ndims)
    PHP_FE(nc_inq_nvars, arginfo_nc_inq_nvars)
    PHP_FE(nc_inq_natts, arginfo_nc_inq_natts)
    PHP_FE(nc_inq_unlimdim, arginfo_nc_inq_unlimdim)

    PHP_FE(nc_sync, NULL)
    PHP_FE(nc_abort, NULL)

    PHP_FE(nc_set_fill, arginfo_nc_set_fill)

    PHP_FE(nc_def_dim, arginfo_nc_def_dim)
    PHP_FE(nc_inq_dimid, arginfo_nc_inq_dimid)

    PHP_FE(nc_inq_dim, arginfo_nc_inq_dim)
    PHP_FE(nc_inq_dimname, arginfo_nc_inq_dimname)
    PHP_FE(nc_inq_dimlen, arginfo_nc_inq_dimlen)

    PHP_FE(nc_rename_dim, NULL)

    PHP_FE(nc_def_var, NULL)
    PHP_FE(nc_inq_var, arginfo_nc_inq_var)
    PHP_FE(nc_inq_varname, NULL)
    PHP_FE(nc_inq_vartype, NULL)
    PHP_FE(nc_inq_varndims, NULL)
    PHP_FE(nc_inq_vardimid, NULL)
    PHP_FE(nc_inq_varnatts, NULL)
    PHP_FE(nc_inq_attname, arginfo_nc_inq_attname)
    PHP_FE(nc_get_att, arginfo_nc_get_att)
    PHP_FE(nc_get_var1, arginfo_nc_get_var1)
    PHP_FE(nc_get_var, NULL)
    PHP_FE(nc_get_vara, arginfo_nc_get_vara)
    PHP_FE(nc_get_vars, NULL)

    PHP_FE(nc_put_var,NULL)
    PHP_FE(nc_put_var_text,NULL)
    PHP_FE(nc_put_var_uchar,NULL)
    PHP_FE(nc_put_var_schar,NULL)
    PHP_FE(nc_put_var_short,NULL)
    PHP_FE(nc_put_var_int,NULL)
    PHP_FE(nc_put_var_long,NULL)
    PHP_FE(nc_put_var_float,NULL)
    PHP_FE(nc_put_var_double,NULL)

    PHP_FE(nc_dump_header,NULL)
    PHP_FE(nc_get_values,NULL)

    {NULL, NULL, NULL}    /* Must be the last line in netcdf_functions[] */
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
    PHP_RINIT(netcdf),        /* Replace with NULL if there's nothing to do at request start */
    PHP_RSHUTDOWN(netcdf),    /* Replace with NULL if there's nothing to do at request end */
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

/* {{{ proto object NetcdfDataset::__construct(string path, string mode, boolean clobber)
   Creates a new netCDF dataset */
PHP_METHOD(NetcdfDataset, __construct)
{
    zend_class_entry * _this_ce;
    zval * _this_zval;

    const char * path = NULL;
    int path_len = 0;
    const char * mode = "r";
    int mode_len = 1;
    zend_bool clobber = 1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|sb", &path, &path_len, &mode, &mode_len, &clobber) == FAILURE) {
        return;
    }

    _this_zval = getThis();
    _this_ce = Z_OBJCE_P(_this_zval);


    do {
        int grpid, ierr, numgrps, numdims, numvars, modeok = 0;
        if (!strcmp(mode, "w"))
        {
            if (clobber)
                ierr = nc_create(path, NC_CLOBBER, &grpid);
            else
                ierr = nc_create(path, NC_NOCLOBBER, &grpid);
            modeok = 1;
        }
        if (!strcmp(mode, "r"))
        {
            ierr = nc_open(path, NC_NOWRITE, &grpid);
            modeok = 1;
        }
        if ((!strcmp(mode, "r")) || (!strcmp(mode, "a")))
        {
            ierr = nc_open(path, NC_WRITE, &grpid);
            modeok = 1;
        }
        if ((!strcmp(mode, "r+s")) || (!strcmp(mode, "as")))
        {
            ierr = nc_open(path, NC_SHARE, &grpid);
            modeok = 1;
        }
        if (!strcmp(mode, "ws"))
        {
            if (clobber)
                ierr = nc_create(path, NC_SHARE | NC_CLOBBER, &grpid);
            else
                ierr = nc_create(path, NC_SHARE | NC_NOCLOBBER, &grpid);
            modeok = 1;
        }
        if (!modeok)
        {
            zend_error(E_ERROR, "mode must be 'w', 'r', 'a' or 'r+')");
            RETURN_FALSE;
        }
        if (ierr != NC_NOERR)
        {
            zend_error(E_ERROR, nc_strerror(ierr));
            RETURN_FALSE;
        }
        add_property_long(getThis(), "_grpid", grpid);
        add_property_string(getThis(), "path", "/");
        add_property_string(getThis(), "filename", path);
    } while (0);
}
/* }}} NetcdfDataset::__construct */

/* {{{ proto void NetcdfDataset::close(void)
   Closes the dataset */
PHP_METHOD(NetcdfDataset, close)
{
    zend_class_entry * _this_ce;

    zval * _this_zval = NULL;

    int ierr;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &_this_zval, NetcdfDataset_entry_ptr) == FAILURE) {
        return;
    }

    _this_ce = Z_OBJCE_P(_this_zval);

    ierr = nc_close(PROP_GET_LONG(_grpid));
    if (ierr != NC_NOERR)
    {
        zend_error(E_ERROR, nc_strerror(ierr));
        RETURN_FALSE;
    }
}
/* }}} NetcdfDataset::close */

/* {{{ proto void NetcdfDataset::sync(void)
   Writes all buffered data in the dataset to the disk file */
PHP_METHOD(NetcdfDataset, sync)
{
    zend_class_entry * _this_ce;

    zval * _this_zval = NULL;

    int ierr;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &_this_zval, NetcdfDataset_entry_ptr) == FAILURE) {
        return;
    }

    _this_ce = Z_OBJCE_P(_this_zval);

    ierr = nc_sync(PROP_GET_LONG(_grpid));
    if (ierr != NC_NOERR)
    {
        zend_error(E_ERROR, nc_strerror(ierr));
        RETURN_FALSE;
    }
}
/* }}} NetcdfDataset::sync */

/* {{{ proto void NetcdfDataset::_redef(void)
   */
PHP_METHOD(NetcdfDataset, _redef)
{
    zend_class_entry * _this_ce;

    zval * _this_zval = NULL;

    int ierr;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &_this_zval, NetcdfDataset_entry_ptr) == FAILURE) {
        return;
    }

    _this_ce = Z_OBJCE_P(_this_zval);

    ierr = nc_redef(PROP_GET_LONG(_grpid));
    if (ierr != NC_NOERR)
    {
        zend_error(E_ERROR, nc_strerror(ierr));
        RETURN_FALSE;
    }
}
/* }}} NetcdfDataset::_redef */

/* {{{ proto void NetcdfDataset::_enddef(void)
   */
PHP_METHOD(NetcdfDataset, _enddef)
{
    zend_class_entry * _this_ce;

    zval * _this_zval = NULL;

    int ierr;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &_this_zval, NetcdfDataset_entry_ptr) == FAILURE) {
        return;
    }

    _this_ce = Z_OBJCE_P(_this_zval);

    ierr = nc_enddef(PROP_GET_LONG(_grpid));
    if (ierr != NC_NOERR)
    {
        zend_error(E_ERROR, nc_strerror(ierr));
        RETURN_FALSE;
    }
}
/* }}} NetcdfDataset::_enddef */

static zend_function_entry php_netcdf_dataset_functions[] = {
    PHP_ME(NetcdfDataset, __construct, NetcdfDataset____construct_args, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(NetcdfDataset, close, NetcdfDataset__close_args, ZEND_ACC_PUBLIC)
    PHP_ME(NetcdfDataset, sync, NetcdfDataset__sync_args, ZEND_ACC_PUBLIC)
    PHP_ME(NetcdfDataset, _redef, NetcdfDataset___redef_args, ZEND_ACC_PUBLIC)
    PHP_ME(NetcdfDataset, _enddef, NetcdfDataset___enddef_args, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(netcdf)
{
  zend_class_entry ce;
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

    INIT_CLASS_ENTRY(ce, PHP_NETCDF_DATASET_NAME, php_netcdf_dataset_functions);
    NetcdfDataset_entry_ptr = zend_register_internal_class(&ce TSRMLS_CC);
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

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slz/", &path, &path_len, &cmode, &zncid) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_create(path, cmode, &ncid);
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

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slz/", &path, &path_len, &mode, &zncid) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_open(path, mode, &ncid);
    ZVAL_LONG(zncid, ncid);

    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_redef(int ncid)
   Puts an open netCDF dataset into define mode */
PHP_FUNCTION(nc_redef)
{
    long ncid, result;

    if ((ZEND_NUM_ARGS() != 1) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ncid) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_redef(ncid);

    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_enddef(int ncid)
   Takes an open netCDF dataset out of define mode */
PHP_FUNCTION(nc_enddef)
{
    long ncid, result;

    if ((ZEND_NUM_ARGS() != 1) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ncid) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_enddef(ncid);

    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto string nc_close(int ncid)
   Closes an open netCDF dataset */
PHP_FUNCTION(nc_close)
{
    long ncid, result;

    if ((ZEND_NUM_ARGS() != 1) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ncid) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_close(ncid);

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

    if ((ZEND_NUM_ARGS() != 5) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz/z/z/z/", &ncid, &zndims, &znvars, &zngatts, &zunlimdimid) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid);
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

    if ((ZEND_NUM_ARGS() != 2) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz/", &ncid, &zndims) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_ndims(ncid, &ndims);
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

    if ((ZEND_NUM_ARGS() != 2) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz", &ncid, &znvars) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_nvars(ncid, &nvars);
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

    if ((ZEND_NUM_ARGS() != 2) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz/", &ncid, &zngatts) != SUCCESS)) {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_natts(ncid, &ngatts);
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

    if ((ZEND_NUM_ARGS() != 2) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz", &ncid, &zunlimdimid) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_unlimdim(ncid, &unlimdimid);
    ZVAL_LONG(zunlimdimid, unlimdimid);

    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_sync(int ncid)
   Synchronizes the disk copy of a netCDF dataset with in-memory buffers */
PHP_FUNCTION(nc_sync)
{
    long ncid, result;

    if ((ZEND_NUM_ARGS() != 1) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ncid) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_sync(ncid);

    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_abort(int ncid)
   Backs out of recent definitions */
PHP_FUNCTION(nc_abort)
{
    long ncid, result;

    if ((ZEND_NUM_ARGS() != 1) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ncid) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_abort(ncid);

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

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz/", &ncid, &fillmode, &zold_mode) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_set_fill(ncid, fillmode, &old_mode);
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

    if ((ZEND_NUM_ARGS() != 4) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lslz/", &ncid, &name, &namelen, &len, &zdimid) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_def_dim(ncid, name, len, &dimid);
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

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsz/", &ncid, &name, &namelen, &zdimid) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_dimid(ncid, name, &dimid);
    ZVAL_LONG(zdimid, dimid);

    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_dim(int ncid, int dimid, string &name, int &length)
   Returns information about an open netCDF dimension */

PHP_FUNCTION(nc_inq_dim)
{
    long ncid, dimid, result;
    size_t length;
    char name[NC_MAX_NAME];
    zval *zname, *zlength;

    if ((ZEND_NUM_ARGS() != 4) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz/z/", &ncid, &dimid, &zname, &zlength) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_dim(ncid, dimid, name, &length);
    ZVAL_STRING(zname, name);
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

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz/", &ncid, &dimid, &zname) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_dimname(ncid, dimid, name);
    ZVAL_STRING(zname, name);

    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_dimlen(int ncid, int dimid, int &length)
   Returns information about an open netCDF dimension */

PHP_FUNCTION(nc_inq_dimlen)
{
    long ncid, dimid, result;
    size_t length;
    zval *zlength;

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz/", &ncid, &dimid, &zlength) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_dimlen(ncid, dimid, &length);
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

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lls", &ncid, &dimid, &name, &namelen) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_rename_dim(ncid, dimid, name);

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
    zval *data;
    HashTable *arr_hash;
    HashPosition pointer;

    if ((ZEND_NUM_ARGS() != 6) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
        "lsllaz/", &ncid, &name, &namelen, &xtype, &ndims, &zdimids, &zvarid) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    arr_hash = Z_ARRVAL_P(zdimids);
    for (zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
        (i<ndims) && ((data = zend_hash_get_current_data_ex(arr_hash, &pointer)) != NULL); i++)
    {
        if (Z_TYPE_P(data) == IS_LONG)
        {
            dimids[i] = Z_LVAL_P(data);
        }
        zend_hash_move_forward_ex(arr_hash, &pointer);
    }
    result = nc_def_var(ncid, name, xtype, ndims, dimids, &varid);
    ZVAL_LONG(zvarid, varid);

    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_var(int ncid, int varid, string &name, int *xtypep, int *ndimsp, int *dimidsp, int *nattsp); */
PHP_FUNCTION(nc_inq_var)
{
    long ncid, varid, result;
    char name[NC_MAX_NAME];
    int dimids[NC_MAX_VAR_DIMS];
    int ndims, nattsp, i;
    nc_type xtype;
    zval *zname, *zxtype, *zndims, *zdimids, *znattsp;

    if ((ZEND_NUM_ARGS() != 7) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz/z/z/z/z/",
        &ncid, &varid, &zname, &zxtype, &zndims, &zdimids, &znattsp) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_var(ncid, varid, name, &xtype, &ndims, dimids, &nattsp);
    if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting the var information", result);

    array_init(zdimids);
    for(i = 0; i < ndims; i++)
    {
        add_index_long(zdimids, i, dimids[i]);
    }

    ZVAL_STRING(zname, name);
    ZVAL_LONG(zxtype, xtype);
    ZVAL_LONG(zndims, ndims);
    ZVAL_LONG(znattsp, nattsp);

    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_varid(int ncid, string name, int &varid)
   Returns the ID of a netCDF variable, given its name */

PHP_FUNCTION(nc_inq_varid)
{
    int ncid, varid, namelen, result;
    char *name = NULL;
    zval *zvarid;

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsz/", &ncid, &name, &namelen, &zvarid) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_varid(ncid, name, &varid);
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

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz/", &ncid, &varid, &zname) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_varname(ncid, varid, name);
    ZVAL_STRING(zname, name);

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

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz/", &ncid, &varid, &zxtype) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_vartype(ncid, varid, &xtype);
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

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz/", &ncid, &varid, &zndims) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_varndims(ncid, varid, &ndims);
    ZVAL_LONG(zndims, ndims);

    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_vardimid(int ncid, int varid, int* dimidsp); */
PHP_FUNCTION(nc_inq_vardimid)
{
    long ncid, varid, result;
    int *dimids = NULL;
    int ndims, i;
    zval *zdimids;

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz", &ncid, &varid, &zdimids) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_varndims(ncid, varid, &ndims);
    if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting the number of dimensions", result);

    dimids = (int*)emalloc(ndims * sizeof (int));
    if (dimids == NULL) RETURN_ERROR("error allocating memory to get the number of dimensions");

    result = nc_inq_vardimid(ncid, varid, dimids);
    if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting the number of dimensions", result);

    array_init(zdimids);
    for (i = 0; i < ndims; i++)
    {
        add_index_long(zdimids, i, dimids[i]);
    }

    efree(dimids);
    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_varnatts(int ncid, int varid, int *nattsp); */
PHP_FUNCTION(nc_inq_varnatts)
{
    long ncid, varid, result;
    int nattsp;
    zval *znattsp;

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz/", &ncid, &varid, &znattsp) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_varnatts(ncid, varid, &nattsp);
    ZVAL_LONG(znattsp, nattsp);

    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_inq_attname(int ncid, int varid, int attnum, string &name)
   Returns information about a netCDF variable */

PHP_FUNCTION(nc_inq_attname)
{
    long ncid, varid, attnum, result;
    char name[NC_MAX_NAME];
    zval *zname;

    if ((ZEND_NUM_ARGS() != 4) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lllz/", &ncid, &varid, &attnum, &zname) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_attname(ncid, varid, attnum, name);
    ZVAL_STRING(zname, name);

    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_get_att(int ncid, int varid, string name, void *value)
   Returns the value of a netCDF attribute */

PHP_FUNCTION(nc_get_att)
{
    int ncid, varid, namelen, result;
    char *name = NULL;
    nc_type at_type;   /* attribute type   */
    size_t  at_len;    /* attribute length */
    zval *zvalue;
    void *value = NULL;

    if ((ZEND_NUM_ARGS() != 4) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llsz/", &ncid, &varid, &name, &namelen, &zvalue) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_att (ncid, varid, name, &at_type, &at_len);
    if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting attribute information (nc_inq_att)", result);

    /* allocate required space before retrieving values */
    value = allocate_space(at_type,at_len);
    if (value == NULL) RETURN_ERROR("error allocating memory to get attribute value");

    result = nc_get_att(ncid, varid, name, value);
    if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting attribute value", result);

    /* assign value/s to zvalue */
    assign_zval(at_type, at_len, value, zvalue);

    efree(value);
    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_get_var1_float(int ncid, int varid, unsigned int *indexp, void *ip); */
PHP_FUNCTION(nc_get_var1)
{
    int i, ndims, result;
    long ncid;
    long varid;
    nc_type xtype;
    size_t *indexp = NULL;
    zval *zindexp, *zvalue;
    void *value = NULL;

    if ((ZEND_NUM_ARGS() != 4) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llaz/", &ncid, &varid, &zindexp, &zvalue) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_varndims(ncid, varid, &ndims);
    if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting variable dimension number",result);

    result = nc_inq_vartype(ncid, varid, &xtype);
    if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting variable type",result);

    indexp = (size_t*)emalloc(ndims * sizeof (size_t));
    if (indexp == NULL) RETURN_ERROR("error allocating memory to get indices");

    if (assign_value(NC_INT, ndims, zindexp, (void *)indexp) == 0)
    {
        RETURN_ERROR("error reading indexp array");
    }

    /* allocate required space before retrieving value */
    value = allocate_space(xtype,1);
    if (value == NULL) RETURN_ERROR("error allocating memory to get value");

    result = nc_get_var1(ncid, varid, indexp, value);
    assign_zval(xtype, 1, value, zvalue);

    efree(value);
    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_get_var(int ncid, int varid, void *ip); */
PHP_FUNCTION(nc_get_var)
{
    long ncid, varid, result;
    char name[NC_MAX_NAME];
    int dimids[NC_MAX_VAR_DIMS];
    int ndims, nattsp, i;
    size_t dim_length, var_length = 1;
    nc_type xtype;
    zval *zvalues;
    void *values = NULL;

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz/", &ncid, &varid, &zvalues) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_var(ncid, varid, name, &xtype, &ndims, dimids, &nattsp);
    if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting the var information", result);

    for (i = 0; i < ndims; i++) {
        result = nc_inq_dimlen(ncid, dimids[i], &dim_length);
        if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting the dimension information", result);
        var_length *= dim_length;
    }

    /* allocate required space before retrieving values */
    values = allocate_space(xtype, var_length);
    if (values == NULL) RETURN_ERROR("error allocating memory to get values");

 
    result = nc_get_var(ncid, varid, values);
    assign_zval(xtype, var_length, values, zvalues);

    efree(values);
    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_get_vara  (int ncid, int varid, const size_t start[],const size_t count[], void *ip); */
PHP_FUNCTION(nc_get_vara)
{
    long ncid, varid, result;
    char name[NC_MAX_NAME];
    int dimids[NC_MAX_VAR_DIMS];
    int ndims, nattsp, dim_length, i;
    size_t var_length = 1;
    size_t *start = NULL, *count = NULL;
    nc_type xtype;
    zval *zstart, *zcount, *zvalues;
    void *values = NULL;

    if ((ZEND_NUM_ARGS() != 5) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llaaz/", &ncid, &varid, &zstart, &zcount, &zvalues) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_var(ncid, varid, name, &xtype, &ndims, dimids, &nattsp);
    if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting the var information", result);

    /* reading start array */
    start = (size_t*)emalloc(ndims * sizeof (size_t));
    if (start == NULL) RETURN_ERROR("error allocating memory to get start");

    if (assign_value(NC_INT, ndims, zstart, (void *)start) == 0)
        RETURN_ERROR("error reading start array");

    /* reading count array */
    count = (size_t*)emalloc(ndims * sizeof (size_t));
    if (count == NULL) RETURN_ERROR("error allocating memory to get count");

    if (assign_value(NC_INT, ndims, zcount, (void *)count) == 0)
    {
        RETURN_ERROR("error reading count array");
    }

    for (i = 0; i < ndims; i++)
    {
        var_length *= count[i];
    }

    /* allocate required space before retrieving values */
    values = allocate_space(xtype, var_length);
    if (values == NULL) RETURN_ERROR("error allocating memory to get values");

    result = nc_get_vara(ncid, varid, start, count, values);
    assign_zval(xtype, var_length, values, zvalues);

    efree(values);
    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_get_vars  (int ncid, int varid, const size_t start[],const size_t count[],const ptrdiff_t stride[], void *ip); */
PHP_FUNCTION(nc_get_vars)
{
    long ncid, varid, result;
    char name[NC_MAX_NAME];
    int dimids[NC_MAX_VAR_DIMS];
    int ndims, nattsp, dim_length, i;
    size_t var_length = 1;
    size_t *start = NULL, *count = NULL;
    ptrdiff_t *stride = NULL;
    nc_type xtype;
    zval *zstart, *zcount, *zstride, *zvalues;
    void *values = NULL;

    if ((ZEND_NUM_ARGS() != 6) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llaaaz/",
        &ncid, &varid, &zstart, &zcount, &zstride, &zvalues) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_var(ncid, varid, name, &xtype, &ndims, dimids, &nattsp);
    if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting the var information", result);

    /* reading start array */
    start = (size_t*)emalloc(ndims * sizeof (size_t));
    if (start == NULL) RETURN_ERROR("error allocating memory to get start");

    if (assign_value(NC_INT, ndims, zstart, (void *) start) == 0)
    {
        RETURN_ERROR("error reading start array");
    }

    /* reading count array */
    count = (size_t*)emalloc(ndims * sizeof (size_t));
    if (count == NULL) RETURN_ERROR("error allocating memory to get count");

    if (assign_value(NC_INT, ndims, zcount, (void *) count) == 0)
    {
        RETURN_ERROR("error reading count array");
    }

    /* reading stride array */
    stride = (ptrdiff_t*)emalloc(ndims * sizeof (ptrdiff_t));
    if (stride == NULL) RETURN_ERROR("error allocating memory to get stride");

    if(assign_value(NC_INT, ndims, zstride, (void *) stride) == 0)
    {
        RETURN_ERROR("error reading stride array");
    }


    for (i = 0; i < ndims; i++)
    {
        var_length *= count[i];
    }

    /* allocate required space before retrieving values */
    values = allocate_space(xtype, var_length);
    if (values == NULL) RETURN_ERROR("error allocating memory to get values"); 

    result = nc_get_vars(ncid, varid, start, count, stride, values);
    assign_zval(xtype, var_length, values, zvalues);

    efree(values);
    RETURN_LONG(result);
}
/* }}} */


/* {{{ proto int nc_put_var(int ncid, int varid, void *values); */
PHP_FUNCTION(nc_put_var)
{
    long ncid, varid, result;
    char name[NC_MAX_NAME];
    int dimids[NC_MAX_VAR_DIMS];
    int ndims, nattsp, i;
    size_t dim_length, var_length = 1;
    nc_type xtype;
    zval *zvalues;
    void *values = NULL;

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lla", &ncid, &varid, &zvalues) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq_var(ncid, varid, name, &xtype, &ndims, dimids, &nattsp);
    if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting the var information", result);

    for(i = 0; i < ndims; i++){
        result = nc_inq_dimlen(ncid, dimids[i], &dim_length);
        if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting the dimension information", result);
        var_length *= dim_length;
    }

    /* allocate required space before retrieving values */
    values = allocate_space(xtype, var_length);
    if (values == NULL) RETURN_ERROR("error allocating memory to get values");

    if (assign_value(xtype, var_length, zvalues, values) == 0)
    {
        RETURN_ERROR("error reading values to write");
    }

    result = nc_put_var(ncid, varid, values);
    efree(values);
    RETURN_LONG(result);
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
    {
        return;
    }

    result = nc_put_var_text(ncid, varid, blob);

    RETURN_LONG(result);
}
/* }}} */



/* {{{ proto int nc_put_var_uchar(int ncid, int varid, const uchar values[]); */
PHP_FUNCTION(nc_put_var_uchar)
{
    long ncid, varid, result;
    zval *zvalues;
    void *values = NULL;

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lla", &ncid, &varid, &zvalues) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    values = get_values_from_array(NC_BYTE, zvalues);
    if (values == NULL) RETURN_NULL();

    result = nc_put_var_uchar(ncid, varid, (unsigned char *)values);
    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_put_var_schar(int ncid, int varid, string &blob, &bloblen); */
PHP_FUNCTION(nc_put_var_schar)
{
    long ncid, varid, result;
    zval *zvalues;
    void *values = NULL;

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lla", &ncid, &varid, &zvalues) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    values = get_values_from_array(NC_BYTE, zvalues);
    if (values == NULL) RETURN_NULL();

    result = nc_put_var_schar(ncid, varid, (char *)values);
    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_put_var_short(int ncid, int varid, string &blob, &bloblen); */
PHP_FUNCTION(nc_put_var_short)
{
    long ncid, varid, result;
    zval *zvalues;
    void *values = NULL;

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lla", &ncid, &varid, &zvalues) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    values = get_values_from_array(NC_SHORT, zvalues);
    if (values == NULL) RETURN_NULL();

    result = nc_put_var_short(ncid, varid, (short *)values);
    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_put_var_int(int ncid, int varid, string &blob, &bloblen); */
PHP_FUNCTION(nc_put_var_int)
{
    long ncid, varid, result;
    zval *zvalues;
    void *values = NULL;

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lla", &ncid, &varid, &zvalues) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    values = get_values_from_array(NC_INT, zvalues);
    if (values == NULL) RETURN_NULL();

    result = nc_put_var_int(ncid, varid, (int *)values);
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
    {
        return;
    }

    result = nc_put_var_long(ncid, varid, blob);

    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_put_var_float(int ncid, int varid, string &blob, &bloblen); */
PHP_FUNCTION(nc_put_var_float)
{
    long ncid, varid, result;
    zval *zvalues;
    void *values = NULL;

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lla", &ncid, &varid, &zvalues) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    values = get_values_from_array(NC_FLOAT, zvalues);
    if (values == NULL) RETURN_NULL();

    result = nc_put_var_float(ncid, varid, (float *)values);
    RETURN_LONG(result);
}
/* }}} */

/* {{{ proto int nc_put_var_double(int ncid, int varid, string &blob, &bloblen); */
PHP_FUNCTION(nc_put_var_double)
{
    long ncid, varid, result;
    zval *zvalues;
    void *values = NULL;

    if ((ZEND_NUM_ARGS() != 3) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lla", &ncid, &varid, &zvalues) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    values = get_values_from_array(NC_DOUBLE, zvalues);
    if (values == NULL) RETURN_NULL();

    result = nc_put_var_double(ncid, varid, (double *)values);
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
    {
        WRONG_PARAM_COUNT;
    }

    result = (char *) nc_strerror(ncerr);

    zend_string *str = zend_string_init(result, strlen(result), 0);
    RETURN_STR(str);
}
/* }}} */

/* {{{ proto string nc_inq_libvers()
   Returns a string identifying the version of the netCDF library, and when it was built */

PHP_FUNCTION(nc_inq_libvers)
{
    char* result;
    zval *zlibvers;

    if (ZEND_NUM_ARGS())
    {
        WRONG_PARAM_COUNT;
    }

    result = (char *) nc_inq_libvers();

    zend_string *str = zend_string_init(result, strlen(result), 0);
    RETURN_STR(str);
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
    {
        WRONG_PARAM_COUNT;
    }

    result = (char *) netcdf_types[xtype];

    zend_string *str = zend_string_init(result, strlen(result), 0);
    RETURN_STR(str);
}
/* }}} */

/* {{{ proto int nc_dump_header(int ncid, int &header)
    Returns header information about an open netCDF dataset:
    ncid    - Netcdf ID
    header  - Array structured with the header info.
              The array has the next structure:
    Array
    (
    [dimensions] => Array
        (
            [dim1] => 64
            [dim2] => 3
            [dim3] => 3
        )

    [unlimdim] => dim3          // it doesn't exist in case of no unlimited variable
    [variables] => Array
        (
            [0] => Array
                (
                    [name] => Var1
                    [type] => NC_FLOAT
                    [dimensions] => Array
                        (
                            [dim3] => 3             // dimension lengths info is redundant to make easy
                            [dim1] => 64            // other tasks
                        )

                    [attributes] => Array
                        (
                            [units] => secons
                            [long_name] => Variable 1
                        )
                )
            [1] => Array
            ...
        )
    [global attributes] => Array
        (
            [attribute1] =>  value_attribute1
            [attribute2] =>  value_attribute2
            [attribute3] =>  value_attribute3
            ...
            )
       )
   */

PHP_FUNCTION(nc_dump_header)
{
    long ncid, result;
    int i, j, nc_ndims, nc_nvars, nc_ngatts, unlimdimid, ndims, nattsp;
    size_t at_len, length;
    nc_type var_type, at_type;
    char name[NC_MAX_NAME];
    char *nc_dimnames[NC_MAX_DIMS];
    int nc_dimlengths[NC_MAX_DIMS];
    int dimids[NC_MAX_VAR_DIMS];
    zval *zheader;
    zval subarray, var_array, dim_array, att_array, zvalue;
    void *value = NULL;

    if ((ZEND_NUM_ARGS() != 2) || (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz/", &ncid, &zheader) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq(ncid, &nc_ndims, &nc_nvars, &nc_ngatts, &unlimdimid);
    if (result != NC_NOERR)
    {
        RETURN_NETCDF_ERROR("error getting netCDF information (nc_inq)", result);
    }

    array_init(zheader);

    /* Getting dimensions array */
    array_init(&subarray);
    for (i = 0; i < nc_ndims; i++) {
        result = nc_inq_dim(ncid, i, name, &length);
        if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting dimension information (nc_inq_dim)", result);

        nc_dimnames[i] = estrdup(name);          /* we keep them for the variables */
        nc_dimlengths[i] = length;               /* we keep them for the variables */
        add_assoc_long(&subarray, name, length);
    }
    add_assoc_zval(zheader, "dimensions", &subarray);

    if (unlimdimid != -1) {
        result = nc_inq_dim(ncid, unlimdimid, name, &length);
        if (result != NC_NOERR)
            RETURN_NETCDF_ERROR("error getting dimension information (nc_inq_dim)", result);

        add_assoc_string(zheader, "unlimdim", name);
    }

    /* Getting variables array */
    array_init(&subarray);
    for (i = 0; i < nc_nvars; i++) {
        result = nc_inq_varname(ncid, i, name);
        if (result != NC_NOERR)
            RETURN_NETCDF_ERROR("error getting variable name (nc_inq_varname)",result);

        result = nc_inq_var(ncid, i, name, &var_type, &ndims, dimids, &nattsp);
        if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting variable information (nc_inq_var)", result);

        array_init(&var_array);

        add_assoc_string(&var_array, "name", name);
        add_assoc_string(&var_array, "type", netcdf_types[var_type]);

        array_init(&dim_array);
        for (j = 0; j < ndims; j++)
            add_assoc_long(&dim_array, nc_dimnames[dimids[j]], nc_dimlengths[dimids[j]]);

        add_assoc_zval(&var_array, "dimensions", &dim_array);

        /* Getting attributes array of a variable */
        array_init(&att_array);
        for (j = 0; j < nattsp; j++) {
            result = nc_inq_attname(ncid, i, j, name);
            if (result != NC_NOERR)
                RETURN_NETCDF_ERROR("error getting attribute name (nc_inq_attname)", result);

            result = nc_inq_att(ncid, i, name, &at_type, &at_len);
            if (result != NC_NOERR)
                RETURN_NETCDF_ERROR("error getting attribute information (nc_inq_att)", result);

            value = allocate_space(at_type, at_len);
            if (value == NULL)
                RETURN_ERROR("error allocating memory to get attribute value");

            result = nc_get_att(ncid, i, name, value);
            if (result != NC_NOERR)
                RETURN_NETCDF_ERROR("error getting attribute value (nc_get_att)", result);

            assign_zval(at_type, at_len, value, &zvalue);
            efree(value);
            add_assoc_zval(&att_array, name, &zvalue);
        }
        add_assoc_zval(&var_array, "attributes", &att_array);

        add_index_zval(&subarray, i, &var_array);
    }
    add_assoc_zval(zheader, "variables", &subarray);

    /* Getting global attributes */
    array_init(&subarray);
    for(i = 0; i < nc_ngatts; i++) {
        result = nc_inq_attname(ncid, NC_GLOBAL, i, name);
        if (result != NC_NOERR)
            RETURN_NETCDF_ERROR("error getting attribute name (nc_inq_attname)", result);
        result = nc_inq_att(ncid, NC_GLOBAL, name, &at_type, &at_len);
        if (result != NC_NOERR)
            RETURN_NETCDF_ERROR("error getting attribute information (nc_inq_att)", result);

        value = allocate_space(at_type,at_len);
        if (value == NULL)
            RETURN_ERROR("error allocating memory to get attribute value");

        result = nc_get_att(ncid, NC_GLOBAL, name, value);
        if (result != NC_NOERR)
            RETURN_NETCDF_ERROR("error getting attribute value (nc_get_att)", result);

        assign_zval(at_type, at_len, value, &zvalue);
        efree(value);
        add_assoc_zval(&subarray, name, &zvalue);
    }
    add_assoc_zval(zheader, "global attributes", &subarray);

    RETURN_LONG(result);
}
/* }}} */

int min(int a, int b)
{
    return (a < b ? a : b);
}

int add_values(int ncid, int varid, int nc_dimlengths[], int scs_lengths[], size_t *start, size_t *count, ptrdiff_t *stride, zval *zvalues) {
    int i, j, ndims, nattsp, var_length = 1, result;
    nc_type xtype;
    char name[NC_MAX_NAME];
    int dimids[NC_MAX_VAR_DIMS];
    size_t *tmp_start = NULL, *tmp_count = NULL;
    ptrdiff_t *tmp_stride = NULL;
    void *values = NULL;
    zval var_values;

    result = nc_inq_var(ncid, varid, name, &xtype, &ndims, dimids, &nattsp);
    if (result != NC_NOERR)
    {
        zend_error(E_WARNING, "%s: error getting the var information\n         %s", __func__, nc_strerror(result));
        return result;
    }

    array_init(&var_values);

    /* simple case. all values */
    if (start == NULL && count == NULL && stride == NULL)
    {
        for(i = 0; i < ndims; i++)
        {
            var_length *= nc_dimlengths[dimids[i]];
        }

        values = allocate_space(xtype, var_length);
        if (values == NULL)
        {
            ISSUE_WARNING("error allocating memory to get values");
            return -1;
        }
        result = nc_get_var(ncid, varid, values);
        assign_zval(xtype, var_length, values, &var_values);
        efree(values);
        add_assoc_zval(zvalues, name, &var_values);
        return NC_NOERR;
    }

    /* init tmp_start, tmp_count and tmp_stride to default values */
    tmp_start  = (size_t*) emalloc(ndims * sizeof (size_t));
    tmp_count  = (size_t*) emalloc(ndims * sizeof (size_t));
    tmp_stride = (ptrdiff_t*) emalloc(ndims * sizeof (ptrdiff_t));
    if (tmp_start == NULL || tmp_count == NULL || tmp_stride == NULL)
    {
        ISSUE_WARNING("error allocating memory");
        return -1;
    }

    for (i = 0; i < ndims; i++)
    {
        tmp_start[i] = 0;
        tmp_count[i] = nc_dimlengths[dimids[i]];
        tmp_stride[i] = 1;
    }

    /* update tmp_start, tmp_count and tmp_stride to user values */
    if (start != NULL)
        for (i = 0; (i < ndims && i < scs_lengths[0]); i++)
            tmp_start[i] = start[i];

    if (stride != NULL)
        for(i = 0; (i < ndims && i < scs_lengths[2]); i++)
            tmp_stride[i] = stride[i];

    /* adjustment of count due to the stride */
    for (i = 0; i < ndims; i++)
            tmp_count[i] = 1 + (int) ((tmp_count[i] - 1)/ tmp_stride[i]);

    if (count != NULL)
        for (i = 0; (i < ndims && i < scs_lengths[1]); i++)
            if (count[i] != -1)
                tmp_count[i] = min(count[i], tmp_count[i]);

    for (i = 0; i < ndims; i++)
        var_length *= tmp_count[i];

    /* allocate required space before retrieving values */
    values = allocate_space(xtype, var_length);
    if (values == NULL)
    {
        ISSUE_WARNING("error allocating memory to get values");
        return -1;
    }

    result = nc_get_vars(ncid, varid, tmp_start, tmp_count, tmp_stride, values);
    assign_zval(xtype, var_length, values, &var_values);
    efree(values);

    add_assoc_zval(zvalues, name, &var_values);
    return result;
}


/* {{{ proto int nc_get_values(int ncid, void *values, [char* var_names[], const size_t start[],const size_t count[],const ptrdiff_t stride[]])
    Returns an array with the values:
    ncid          - Netcdf ID
    values        - PHP value where will be set the output array
    var_names[]   - Array with the varables names to filter. If NULL or not included, all variables are in the output
    start[]       - Array with start parameter. If NULL or not included, default is (0,0,..).
                    In case that be (N), and the variable be multidimensional. It will be set to (N,0,0,...) for that variable
                    In case that be (N,M) and the variable be unidimensional. It will be set to (N) for that variable
    count[]       - Array with count parameter. If NULL or not included, default is (1,1,..).
                    In case that be (N), and the variable be multidimensional. It will be set to (N,1,1,...) for that variable
                    In case that be (N,M) and the variable be unidimensional. It will be set to (N) for that variable
                    In case that any component be set to -1, it will be calculated to the maximum value depending on its dimension.
    stride[]      - Array with stride parameter. If NULL or not included, default is (1,1,..).
                    In case that be (N), and the variable be multidimensional. It will be set to (N,1,1,...) for that variable
                    In case that be (N,M) and the variable be unidimensional. It will be set to (N) for that variable
   */
PHP_FUNCTION(nc_get_values)
{
    long ncid, result;
    int i, j, nc_ndims, nc_nvars, nc_ngatts, unlimdimid;
    size_t length;
    int nc_dimlengths[NC_MAX_DIMS];
    int scs_lengths[] = {0, 0, 0};
    int *varids = NULL, varid, var_count;
    size_t *start = NULL, *count = NULL;
    ptrdiff_t *stride = NULL;
    zval *zvalues, *zvar_names = NULL, *zstart = NULL, *zcount = NULL, *zstride = NULL;
    zval *data;
    HashTable *arr_hash;
    HashPosition pointer;

    if ((ZEND_NUM_ARGS() < 2 || ZEND_NUM_ARGS() > 6) ||
        (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz|a!a!a!a!",
        &ncid, &zvalues, &zvar_names, &zstart, &zcount, &zstride) != SUCCESS))
    {
        WRONG_PARAM_COUNT;
    }

    result = nc_inq(ncid, &nc_ndims, &nc_nvars, &nc_ngatts, &unlimdimid);
    if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting netcdf information (nc_inq)", result);

    /* Getting dimensions length */
    for (i = 0; i < nc_ndims; i++)
    {
        result = result = nc_inq_dimlen(ncid, i, &length); /* wtf? */
        if (result != NC_NOERR) RETURN_NETCDF_ERROR("error getting dimension information (nc_inq_dimlen)", result);
        nc_dimlengths[i] = length;               /* we get them for the variables */
    }

    /* reading var_names array and initialising varids array*/
    if (zvar_names != NULL)
    {
        arr_hash = Z_ARRVAL_P(zvar_names);
        varids = (int *)emalloc(zend_hash_num_elements(arr_hash) * sizeof (int));
        if (varids == NULL) RETURN_ERROR("error allocating memory to get var_names");

        for (zend_hash_internal_pointer_reset_ex(arr_hash, &pointer), var_count = 0;
            (data = zend_hash_get_current_data_ex(arr_hash, &pointer)) != NULL;
            zend_hash_move_forward_ex(arr_hash, &pointer))
        {
            if (Z_TYPE_P(data) == IS_STRING)
            {
                result = nc_inq_varid(ncid, Z_STRVAL_P(data), &varid);
                if (result != NC_NOERR) php_error(E_WARNING, "%s: variable \"%s\" is not in the netcdf file", __func__, Z_STRVAL_P(data));
                else varids[var_count++] = varid;
            }
        }
    }

    /* reading start array */
    if (zstart != NULL)
    {
        arr_hash = Z_ARRVAL_P(zstart);
        start = (size_t*)emalloc(zend_hash_num_elements(arr_hash) * sizeof(size_t));
        if (start == NULL) RETURN_ERROR("error allocating memory to get start");
        scs_lengths[0] = zend_hash_num_elements(arr_hash);

        for (zend_hash_internal_pointer_reset_ex(arr_hash, &pointer), i = 0;
            (data = zend_hash_get_current_data_ex(arr_hash, &pointer)) != NULL;
            zend_hash_move_forward_ex(arr_hash, &pointer), i++)
        {
            if (Z_TYPE_P(data) == IS_LONG)
                start[i] = Z_LVAL_P(data);
        }
    }

    /* reading count array */
    if(zcount != NULL) {
        arr_hash = Z_ARRVAL_P(zcount);
        count = (size_t*) emalloc(zend_hash_num_elements(arr_hash) * sizeof (size_t));
        if (count == NULL) RETURN_ERROR("error allocating memory to get count");
        scs_lengths[1] = zend_hash_num_elements(arr_hash);

        for (zend_hash_internal_pointer_reset_ex(arr_hash, &pointer), i = 0;
            (data = zend_hash_get_current_data_ex(arr_hash, &pointer)) != NULL;
            zend_hash_move_forward_ex(arr_hash, &pointer), i++)
        {
            if (Z_TYPE_P(data) == IS_LONG)
                count[i] = Z_LVAL_P(data);
        }
    }

    /* reading stride array */
    if(zstride != NULL) {
        arr_hash = Z_ARRVAL_P(zstride);
        stride = (ptrdiff_t*)emalloc(zend_hash_num_elements(arr_hash) * sizeof (ptrdiff_t));
        if (stride == NULL) RETURN_ERROR("error allocating memory to get stride");
        scs_lengths[2] = zend_hash_num_elements(arr_hash);

        for (zend_hash_internal_pointer_reset_ex(arr_hash, &pointer), i = 0;
            (data = zend_hash_get_current_data_ex(arr_hash, &pointer)) != NULL;
            zend_hash_move_forward_ex(arr_hash, &pointer), i++)
        {
            if (Z_TYPE_P(data) == IS_LONG)
                stride[i] = Z_LVAL_P(data);
        }
    }

    array_init(zvalues);
    if (varids == NULL) {
        for (i = 0; i < nc_nvars; i++)
            if (add_values(ncid, i, nc_dimlengths, scs_lengths, start, count, stride, zvalues) != NC_NOERR)
                RETURN_NULL();
    }
    else {
        for (i = 0; i < var_count; i++)
            if (add_values(ncid, varids[i], nc_dimlengths, scs_lengths, start, count, stride, zvalues) != NC_NOERR)
                RETURN_NULL();
    }

    RETURN_LONG(NC_NOERR);
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: et sw=4 ts=4 fdm=marker
 * vim<600: et sw=4 ts=4
 */
