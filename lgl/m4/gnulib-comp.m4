# DO NOT EDIT! GENERATED AUTOMATICALLY!
# Copyright (C) 2004-2007 Free Software Foundation, Inc.
#
# This file is free software, distributed under the terms of the GNU
# General Public License.  As a special exception to the GNU General
# Public License, this file may be distributed as part of a program
# that contains a configuration script generated by Autoconf, under
# the same distribution terms as the rest of that program.
#
# Generated by gnulib-tool.
#
# This file represents the compiled summary of the specification in
# gnulib-cache.m4. It lists the computed macro invocations that need
# to be invoked from configure.ac.
# In projects using CVS, this file can be treated like other built files.


# This macro should be invoked from ./configure.in, in the section
# "Checks for programs", right after AC_PROG_CC, and certainly before
# any checks for libraries, header files, types and library functions.
AC_DEFUN([lgl_EARLY],
[
  m4_pattern_forbid([^gl_[A-Z]])dnl the gnulib macro namespace
  m4_pattern_allow([^gl_ES$])dnl a valid locale name
  m4_pattern_allow([^gl_LIBOBJS$])dnl a variable
  m4_pattern_allow([^gl_LTLIBOBJS$])dnl a variable
  AC_REQUIRE([AC_PROG_RANLIB])
  AC_REQUIRE([AC_GNU_SOURCE])
  AC_REQUIRE([gl_LOCK_EARLY])
])

# This macro should be invoked from ./configure.in, in the section
# "Check for header files, types and library functions".
AC_DEFUN([lgl_INIT],
[
  m4_pushdef([AC_LIBOBJ], m4_defn([lgl_LIBOBJ]))
  m4_pushdef([AC_REPLACE_FUNCS], m4_defn([lgl_REPLACE_FUNCS]))
  m4_pushdef([AC_LIBSOURCES], m4_defn([lgl_LIBSOURCES]))
  AM_CONDITIONAL([GL_COND_LIBTOOL], [true])
  gl_cond_libtool=true
  gl_source_base='lgl'
    AC_DEFINE([GNULIB_ABSOLUTE_HEADER], [1],
      [Define to 1 when using the gnulib module absolute-header.])
  gl_FUNC_ALLOCA
    AC_DEFINE([GNULIB_ALLOCA_OPT], [1],
      [Define to 1 when using the gnulib module alloca-opt.])
  gl_GC
  if test $gl_cond_libtool = false; then
    gl_ltlibdeps="$gl_ltlibdeps $LTLIBGCRYPT"
    gl_libdeps="$gl_libdeps $LIBGCRYPT"
  fi
    AC_DEFINE([GNULIB_GC], [1],
      [Define to 1 when using the gnulib module gc.])
  gl_GC_ARCFOUR
    AC_DEFINE([GNULIB_GC_ARCFOUR], [1],
      [Define to 1 when using the gnulib module gc-arcfour.])
  gl_GC_ARCTWO
    AC_DEFINE([GNULIB_GC_ARCTWO], [1],
      [Define to 1 when using the gnulib module gc-arctwo.])
  gl_GC_DES
    AC_DEFINE([GNULIB_GC_DES], [1],
      [Define to 1 when using the gnulib module gc-des.])
  gl_GC_HMAC_MD5
    AC_DEFINE([GNULIB_GC_HMAC_MD5], [1],
      [Define to 1 when using the gnulib module gc-hmac-md5.])
  gl_GC_HMAC_SHA1
    AC_DEFINE([GNULIB_GC_HMAC_SHA1], [1],
      [Define to 1 when using the gnulib module gc-hmac-sha1.])
  gl_GC_MD2
    AC_DEFINE([GNULIB_GC_MD2], [1],
      [Define to 1 when using the gnulib module gc-md2.])
  gl_GC_MD4
    AC_DEFINE([GNULIB_GC_MD4], [1],
      [Define to 1 when using the gnulib module gc-md4.])
  gl_GC_MD5
    AC_DEFINE([GNULIB_GC_MD5], [1],
      [Define to 1 when using the gnulib module gc-md5.])
  gl_GC_PBKDF2_SHA1
    AC_DEFINE([GNULIB_GC_PBKDF2_SHA1], [1],
      [Define to 1 when using the gnulib module gc-pbkdf2-sha1.])
  gl_GC_RANDOM
    AC_DEFINE([GNULIB_GC_RANDOM], [1],
      [Define to 1 when using the gnulib module gc-random.])
  gl_GC_RIJNDAEL
    AC_DEFINE([GNULIB_GC_RIJNDAEL], [1],
      [Define to 1 when using the gnulib module gc-rijndael.])
  gl_GC_SHA1
    AC_DEFINE([GNULIB_GC_SHA1], [1],
      [Define to 1 when using the gnulib module gc-sha1.])
  dnl you must add AM_GNU_GETTEXT([external]) or similar to configure.ac.
  AM_GNU_GETTEXT_VERSION([0.15])
    AC_DEFINE([GNULIB_GETTEXT], [1],
      [Define to 1 when using the gnulib module gettext.])
    AC_DEFINE([GNULIB_GETTEXT_H], [1],
      [Define to 1 when using the gnulib module gettext-h.])
    AC_DEFINE([GNULIB_HAVELIB], [1],
      [Define to 1 when using the gnulib module havelib.])
  gl_MD2
    AC_DEFINE([GNULIB_MD2], [1],
      [Define to 1 when using the gnulib module md2.])
  gl_FUNC_MEMMEM
    AC_DEFINE([GNULIB_MEMMEM], [1],
      [Define to 1 when using the gnulib module memmem.])
  gl_FUNC_MEMMOVE
    AC_DEFINE([GNULIB_MEMMOVE], [1],
      [Define to 1 when using the gnulib module memmove.])
  gl_MINMAX
    AC_DEFINE([GNULIB_MINMAX], [1],
      [Define to 1 when using the gnulib module minmax.])
  gl_FUNC_READ_FILE
    AC_DEFINE([GNULIB_READ_FILE], [1],
      [Define to 1 when using the gnulib module read-file.])
  gl_SIZE_MAX
    AC_DEFINE([GNULIB_SIZE_MAX], [1],
      [Define to 1 when using the gnulib module size_max.])
  gl_FUNC_SNPRINTF
    AC_DEFINE([GNULIB_SNPRINTF], [1],
      [Define to 1 when using the gnulib module snprintf.])
  gl_TYPE_SOCKLEN_T
    AC_DEFINE([GNULIB_SOCKLEN], [1],
      [Define to 1 when using the gnulib module socklen.])
  gl_STDINT_H
    AC_DEFINE([GNULIB_STDINT], [1],
      [Define to 1 when using the gnulib module stdint.])
  gl_FUNC_STRVERSCMP
    AC_DEFINE([GNULIB_STRVERSCMP], [1],
      [Define to 1 when using the gnulib module strverscmp.])
  gl_HEADER_SYS_SOCKET
    AC_DEFINE([GNULIB_SYS_SOCKET], [1],
      [Define to 1 when using the gnulib module sys_socket.])
  gl_HEADER_SYS_STAT_H
    AC_DEFINE([GNULIB_SYS_STAT], [1],
      [Define to 1 when using the gnulib module sys_stat.])
  gl_HEADER_UNISTD
    AC_DEFINE([GNULIB_UNISTD], [1],
      [Define to 1 when using the gnulib module unistd.])
  gl_FUNC_VASNPRINTF
    AC_DEFINE([GNULIB_VASNPRINTF], [1],
      [Define to 1 when using the gnulib module vasnprintf.])
  gl_XSIZE
    AC_DEFINE([GNULIB_XSIZE], [1],
      [Define to 1 when using the gnulib module xsize.])
    AC_DEFINE([GNULIB_DUMMY], [1],
      [Define to 1 when using the gnulib module dummy.])
  m4_popdef([AC_LIBSOURCES])
  m4_popdef([AC_REPLACE_FUNCS])
  m4_popdef([AC_LIBOBJ])
  AC_CONFIG_COMMANDS_PRE([
    lgl_libobjs=
    lgl_ltlibobjs=
    if test -n "$lgl_LIBOBJS"; then
      # Remove the extension.
      sed_drop_objext='s/\.o$//;s/\.obj$//'
      for i in `for i in $lgl_LIBOBJS; do echo "$i"; done | sed "$sed_drop_objext" | sort | uniq`; do
        lgl_libobjs="$lgl_libobjs $i.$ac_objext"
        lgl_ltlibobjs="$lgl_ltlibobjs $i.lo"
      done
    fi
    AC_SUBST([lgl_LIBOBJS], [$lgl_libobjs])
    AC_SUBST([lgl_LTLIBOBJS], [$lgl_ltlibobjs])
  ])
])

# Like AC_LIBOBJ, except that the module name goes
# into lgl_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([lgl_LIBOBJ],
  [lgl_LIBOBJS="$lgl_LIBOBJS $1.$ac_objext"])

# Like AC_REPLACE_FUNCS, except that the module name goes
# into lgl_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([lgl_REPLACE_FUNCS],
  [AC_CHECK_FUNCS([$1], , [lgl_LIBOBJ($ac_func)])])

# Like AC_LIBSOURCES, except that it does nothing.
# We rely on EXTRA_lib..._SOURCES instead.
AC_DEFUN([lgl_LIBSOURCES],
  [])

# This macro records the list of files which have been installed by
# gnulib-tool and may be removed by future gnulib-tool invocations.
AC_DEFUN([lgl_FILE_LIST], [
  build-aux/config.rpath
  lib/alloca_.h
  lib/arcfour.c
  lib/arcfour.h
  lib/arctwo.c
  lib/arctwo.h
  lib/asnprintf.c
  lib/des.c
  lib/des.h
  lib/dummy.c
  lib/gc-gnulib.c
  lib/gc-libgcrypt.c
  lib/gc-pbkdf2-sha1.c
  lib/gc.h
  lib/gettext.h
  lib/hmac-md5.c
  lib/hmac-sha1.c
  lib/hmac.h
  lib/md2.c
  lib/md2.h
  lib/md4.c
  lib/md4.h
  lib/md5.c
  lib/md5.h
  lib/memmem.c
  lib/memmem.h
  lib/memmove.c
  lib/memxor.c
  lib/memxor.h
  lib/minmax.h
  lib/printf-args.c
  lib/printf-args.h
  lib/printf-parse.c
  lib/printf-parse.h
  lib/read-file.c
  lib/read-file.h
  lib/rijndael-alg-fst.c
  lib/rijndael-alg-fst.h
  lib/rijndael-api-fst.c
  lib/rijndael-api-fst.h
  lib/sha1.c
  lib/sha1.h
  lib/size_max.h
  lib/snprintf.c
  lib/snprintf.h
  lib/socket_.h
  lib/stat_.h
  lib/stdint_.h
  lib/strverscmp.c
  lib/strverscmp.h
  lib/vasnprintf.c
  lib/vasnprintf.h
  lib/xsize.h
  m4/absolute-header.m4
  m4/alloca.m4
  m4/arcfour.m4
  m4/arctwo.m4
  m4/codeset.m4
  m4/des.m4
  m4/eoverflow.m4
  m4/gc-arcfour.m4
  m4/gc-arctwo.m4
  m4/gc-des.m4
  m4/gc-hmac-md5.m4
  m4/gc-hmac-sha1.m4
  m4/gc-md2.m4
  m4/gc-md4.m4
  m4/gc-md5.m4
  m4/gc-pbkdf2-sha1.m4
  m4/gc-random.m4
  m4/gc-rijndael.m4
  m4/gc-sha1.m4
  m4/gc.m4
  m4/gettext.m4
  m4/glibc2.m4
  m4/glibc21.m4
  m4/hmac-md5.m4
  m4/hmac-sha1.m4
  m4/iconv.m4
  m4/intdiv0.m4
  m4/intl.m4
  m4/intldir.m4
  m4/intmax.m4
  m4/intmax_t.m4
  m4/inttypes-pri.m4
  m4/inttypes_h.m4
  m4/lcmessage.m4
  m4/lib-ld.m4
  m4/lib-link.m4
  m4/lib-prefix.m4
  m4/lock.m4
  m4/longdouble.m4
  m4/longlong.m4
  m4/md2.m4
  m4/md4.m4
  m4/md5.m4
  m4/memmem.m4
  m4/memmove.m4
  m4/memxor.m4
  m4/minmax.m4
  m4/nls.m4
  m4/po.m4
  m4/printf-posix.m4
  m4/progtest.m4
  m4/read-file.m4
  m4/rijndael.m4
  m4/sha1.m4
  m4/size_max.m4
  m4/snprintf.m4
  m4/socklen.m4
  m4/sockpfaf.m4
  m4/stdint.m4
  m4/stdint_h.m4
  m4/strverscmp.m4
  m4/sys_socket_h.m4
  m4/sys_stat_h.m4
  m4/uintmax_t.m4
  m4/ulonglong.m4
  m4/unistd_h.m4
  m4/vasnprintf.m4
  m4/visibility.m4
  m4/wchar_t.m4
  m4/wint_t.m4
  m4/xsize.m4
])
