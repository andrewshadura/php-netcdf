dnl $Id$
dnl config.m4 for extension netcdf

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(netcdf, for netcdf support,
dnl Make sure that the comment is aligned:
[  --with-netcdf             Include netcdf support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(netcdf, whether to enable netcdf support,
dnl Make sure that the comment is aligned:
dnl [  --enable-netcdf           Enable netcdf support])

if test "$PHP_NETCDF" != "no"; then
  dnl First try to find pkg-config
  if test -z "$PKG_CONFIG"; then
    AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  fi

  dnl If pkg-config is found try using it
  if test "$PHP_NETCDF_DIR" = "yes" && test -x "$PKG_CONFIG" && $PKG_CONFIG --exists netcdf; then
    if $PKG_CONFIG --atleast-version=4.1.0 netcdf; then
      found_netcdf=yes
      NETCDF_LIBS=`$PKG_CONFIG --libs netcdf`
      NETCDF_INCS=`$PKG_CONFIG --cflags-only-I netcdf`
      NETCDF_INC_DIR=`$PKG_CONFIG --variable=includedir netcdf`
    else
      AC_MSG_ERROR([NetCDF version 4.1.0 or greater required.])
    fi

    if test -n "$NETCDF_LIBS"; then
      PHP_EVAL_LIBLINE($NETCDF_LIBS, $1)
    fi
    if test -n "$NETCDF_INCS"; then
      PHP_EVAL_INCLINE($NETCDF_INCS)
    fi
  fi

  dnl If pkg-config fails for some reason, revert to the old method
  if test "$found_netcdf" = "no"; then
    # --with-netcdf -> check with-path
    SEARCH_PATH="/usr/local /usr"     # you might want to change this
    SEARCH_FOR="/include/netcdf.h"  # you most likely want to change this
    if test -r $PHP_NETCDF/$SEARCH_FOR; then # path given as parameter
      NETCDF_INC_DIR=$PHP_NETCDF/include
    else # search default path list
      AC_MSG_CHECKING([for netcdf.h in default path])
      for i in $SEARCH_PATH ; do
        if test -r $i/$SEARCH_FOR; then
          NETCDF_INC_DIR=$i/include
          AC_MSG_RESULT(found in $i)
        fi
      done
    fi
    dnl
    if test -z "$NETCDF_INC_DIR"; then
      AC_MSG_RESULT([not found])
      AC_MSG_ERROR([Please check the netcdf distribution])
    fi

    SEARCH_PATH="/usr/local /usr"     # you might want to change this
    SEARCH_FOR="/lib/libnetcdf.so"  # you most likely want to change this
    if test -r $PHP_NETCDF/$SEARCH_FOR; then # path given as parameter
      NETCDF_LIB_DIR=$PHP_NETCDF/lib
    else # search default path list
      AC_MSG_CHECKING([for libnetcdf in default path])
      for i in $SEARCH_PATH ; do
        if test -r $i/$SEARCH_FOR; then
          NETCDF_LIB_DIR=$i/lib
          AC_MSG_RESULT(found in $i)
        fi
      done
    fi
    dnl
    if test -z "$NETCDF_LIB_DIR"; then
      AC_MSG_RESULT([not found])
      AC_MSG_ERROR([Please check the netCDF distribution])
    fi
  fi
  # --with-netcdf -> add include path
  PHP_ADD_INCLUDE($NETCDF_INC_DIR)

  # --with-netcdf -> check for lib and symbol presence
  LIBNAME=netcdf # you may want to change this
  dnl O_LDFLAGS=$LDFLAGS
  dnl LDFLAGS="$LDFLAGS -L$NETCDF_LIB_DIR -l$LIBNAME"
  PHP_ADD_LIBRARY($LIBNAME)
  LIBSYMBOL=nc_inq_libvers # you most likely want to change this 

  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
    PHP_ADD_LIBRARY($LIBNAME)
      AC_DEFINE(HAVE_NETCDF,1,[Build netCDF extension])
  ],[
      AC_MSG_ERROR([wrong netCDF library version or lib not found])
  ])
  
  PHP_SUBST(NETCDF_SHARED_LIBADD)
  NETCDF_SHARED_LIBADD=-l$LIBNAME

  PHP_NEW_EXTENSION(netcdf, netcdf.c, $ext_shared)
fi
