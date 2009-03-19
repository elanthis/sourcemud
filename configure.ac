# Source MUD
# Configuration script
BASE_VERSION=0.26
VERSION=0.26.20081030

AC_INIT([Source MUD], 0.26.20081030, [http://sourcemud.org/bugs])
AC_CONFIG_SRCDIR(src/mud/main.cc)
AC_CONFIG_HEADERS(include/config.h)

if test -z "$VERSION" ; then
	VERSION="$PACKAGE_VERSION"
fi

AC_PREREQ(2.52)

AC_PROG_CXX
AC_PROG_CC
AC_C_BIGENDIAN
AC_SUBST(WORDS_BIGENDIAN)

AC_HEADER_STDC
AC_HEADER_TIME
AC_FUNC_VPRINTF

# Look for sendmail
AC_ARG_WITH(sendmail,[  --with-sendmail=<path>  Path of your sendmail bianry],[PATH_SENDMAIL="$withval"],[AC_PATH_PROG(PATH_SENDMAIL, sendmail, none, [$PATH:/usr/sbin:/sbin])])
if test "x$PATH_SENDMAIL" != "xnone" ; then
	AC_DEFINE(HAVE_SENDMAIL, 1, [Sendmail binary is available])
	AC_SUBST(PATH_SENDMAIL)
fi

# Look for perl
AC_ARG_WITH(
	perl,
	[  --with-perl=<path>      Path of your perl bianry],
	[PATH_PERL="$withval"],
	[AC_PATH_PROG(
		PATH_PERL,
		perl,
		none,
		[$PATH:/bin:/usr/bin]
	)]
)
if test "x$PATH_PERL" = "xnone"; then
	AC_MSG_ERROR([Can't find perl])
fi
AC_SUBST(PATH_PERL)

# Backtrace generation
AC_CHECK_HEADER(execinfo.h,[
	AC_DEFINE(HAVE_EXECINFO, 1, [The execinfo.h header is present])
])

# Networking libraries
AC_CHECK_LIB(socket,connect)
AC_CHECK_LIB(nsl,gethostbyname)
AC_CHECK_FUNC(getpeereid,AC_DEFINE(HAVE_GETPEEREID,1,[Have getpeereid() available]))
AC_CHECK_FUNC(connect,,AC_MSG_ERROR([can't find connect()]))
AC_CHECK_FUNC(gethostname,,AC_MSG_ERROR([can't find gethostname()]))
AC_CHECK_FUNC(getnameinfo,AC_DEFINE(HAVE_GETNAMEINFO,1,[Have getnameinfo() available]))
AC_CHECK_FUNC(gethostbyname,,AC_MSG_ERROR([can't find gethostbyname()]))
AC_CHECK_FUNC(inet_pton,AC_DEFINE(HAVE_INET_PTON,1,[Have inet_pton()]))
AC_CHECK_FUNC(inet_ntop,AC_DEFINE(HAVE_INET_NTOP,1,[Have inet_ntop()]))
AC_CHECK_FUNC(poll,AC_DEFINE(HAVE_POLL,1,[Have poll() available]))

# Standard functions
AC_CHECK_FUNC(strdup,,AC_MSG_ERROR([can't find strdup()]))
AC_CHECK_FUNC(strcasecmp,,AC_MSG_ERROR([can't find strcasecmp()]))
AC_CHECK_FUNC(strncasecmp,,AC_MSG_ERROR([can't find strncasecmp()]))
AC_CHECK_FUNC(snprintf,,AC_MSG_ERROR([can't find snprintf()]))
AC_CHECK_FUNC(random,,AC_MSG_ERROR([can't find random()]))
AC_CHECK_FUNC(fnmatch,,AC_MSG_ERROR([can't find fnmatch()]))
AC_CHECK_FUNC(regcomp,,AC_MSG_ERROR([can't find regcomp()]))

# Standard types
AC_CHECK_TYPE(intptr_t,,[AC_MSG_ERROR([Type intptr_t not available])])

# Math library
AC_CHECK_LIB(m,cos,,)

# zlib support
AC_CHECK_LIB(z,deflate,ZLIB=yes,ZLIB=no)
if test "x$ZLIB" = "xyes" ; then
	LIBS="$LIBS -lz"
	AC_DEFINE(HAVE_ZLIB, 1, [We have zlib])
	LIBTELNET_HAVE_ZLIB="-DHAVE_ZLIB"
	AC_SUBST(LIBTELNET_HAVE_ZLIB)
fi

# Debugging, always
AC_DEFINE(DEBUG, 1, [Enable debugging features])

# build date
BUILD_DATE=`date +'%e %B %Y'`
AC_SUBST(BUILD_DATE)

# hostname
if hostname -f >/dev/null 2>&1 ; then
	HOSTNAME=`hostname -f`
else
	HOSTNAME=`hostname`
fi
AC_SUBST(HOSTNAME)

# begin output
AC_SUBST(SX_CFLAGS)
AC_SUBST(SX_LIBS)
AC_SUBST(SX_INCLUDES)
AC_SUBST(SX_LDFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(INCLUDES)
AC_SUBST(LDFLAGS)
AC_SUBST(LIBS)

AC_OUTPUT([
Makefile
sourcemud
tools/replace.sh
])

chmod +x sourcemud

echo
echo "Source MUD"
echo "Copyright (C) 2000,2001,2002,2003,2004,2008  Sean Middleditch"
echo "See COPYING for license details."

echo
echo "  Version    ... $PACKAGE_VERSION"
echo "  Install    ... ${prefix}"
if test "x$IPV6" = "xyes" ; then
	echo "  IPv6       ... enabled"
else
	echo "  IPv6       ... disabled"
fi
if test "x$ZLIB" = "xyes" ; then
	echo "  MCCPv2     ... enabled"
else
	echo "  MCCPv2     ... disabled"
fi
if test "x$PATH_SENDMAIL" != "xnone" ; then
	echo "  Sendmail   ... $PATH_SENDMAIL"
else
	echo "  Sendmail   ... not found"
fi
echo "  Perl       ... $PATH_PERL"

echo
echo "Now type \"make\" to build Source MUD"
echo
