# AweMUD NG
# Configuration script
BASE_VERSION=0.24
VERSION=0.24.20060119

AC_INIT([AweMUD], 0.24.20060119, [http://bugs.awemud.net/submit.php])
AC_CONFIG_SRCDIR(src/mud/main.cc)
AC_CONFIG_AUX_DIR(.)
AM_INIT_AUTOMAKE([1.8])
AM_CONFIG_HEADER(include/config.h)

if test -z "$VERSION" ; then
	VERSION="$PACKAGE_VERSION"
fi

AC_PREREQ(2.52)

AC_PROG_CXX
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_C_BIGENDIAN
AC_SUBST(WORDS_BIGENDIAN)

AC_HEADER_STDC
AC_HEADER_TIME
AC_FUNC_VPRINTF

# Docbook tools
AC_ARG_ENABLE(docs,[  --disable-docs          Disable building of documentation],BUILD_DOCS="$enableval",BUILD_DOCS=yes)
AC_ARG_ENABLE(docs,[  --enable-pdf            Enable building of PDF documentation],BUILD_PDFDOCS="$enableval",BUILD_PDFDOCS=no)
if test "x$BUILD_DOCS" = "xyes" ; then
	AC_PATH_PROG(PATH_XMLTO,xmlto)
	if test "x$PATH_XMLTO" = "x" ; then
		BUILD_DOCS="no"
	fi
	AC_SUBST(PATH_XMLTO)
fi
AM_CONDITIONAL(BUILD_DOCS, test "x$BUILD_DOCS" != "xno")
AM_CONDITIONAL(BUILD_PDFDOCS, test "x$BUILD_PDFDOCS" != "xno")

# XSLT processor
AC_PATH_PROG(PATH_XSLTPROC,xsltproc)
if test "x$PATH_XSLTPROC" = "x" ; then
	AC_MSG_ERROR([could not find xsltproc])
fi

# Look for sendmail
AC_ARG_WITH(sendmail,[  --with-sendmail=<path>  Path of your sendmail bianry],[PATH_SENDMAIL="$withval"],[AC_PATH_PROG(PATH_SENDMAIL, sendmail, none, [$PATH:/usr/sbin:/sbin])])
if test "x$PATH_SENDMAIL" != "xnone" ; then
	AC_DEFINE(HAVE_SENDMAIL, 1, [Sendmail binary is available])
	SENDMAIL=yes
	AC_SUBST(PATH_SENDMAIL)
fi

# Backtrace generation
AC_CHECK_HEADER(execinfo.h,[
	AC_DEFINE(HAVE_EXECINFO, 1, [The execinfo.h header is present])
])

# don't use automake's yacc/lex support - it's totally broken for our needs
AC_PATH_PROG(PATH_BISON,bison)
if test -z "$PATH_BISON" ; then
	AC_MSG_ERROR([can't find bison])
fi
AC_PATH_PROG(PATH_FLEX,flex)
if test -z "$PATH_FLEX" ; then
	AC_MSG_ERROR([can't find flex])
fi
AC_SUBST(PATH_BISON)
AC_SUBST(PATH_FLEX)

# Standard functions
AC_CHECK_FUNC(strdup,,AC_MSG_ERROR([can't find strdup()]))
AC_CHECK_FUNC(strcasecmp,,AC_MSG_ERROR([can't find strcasecmp()]))
AC_CHECK_FUNC(strncasecmp,,AC_MSG_ERROR([can't find strncasecmp()]))
AC_CHECK_FUNC(snprintf,,AC_MSG_ERROR([can't find snprintf()]))
AC_CHECK_FUNC(random,,AC_MSG_ERROR([can't find random()]))
AC_CHECK_FUNC(fnmatch,,AC_MSG_ERROR([can't find fnmatch()]))
AC_CHECK_FUNC(gethostname,,AC_MSG_ERROR([can't find gethostname()]))
AC_CHECK_FUNC(getnameinfo,,AC_MSG_ERROR([can't find getnameinfo()]))
AC_CHECK_FUNC(poll,AC_DEFINE(HAVE_POLL,1,[Have poll() available]))
AC_CHECK_FUNC(getpeereid,AC_DEFINE(HAVE_GETPEEREID,1,[Have getpeereid() available]))
AC_CHECK_TYPE(socklen_t,AC_DEFINE(HAVE_SOCKLEN_T,1,[Have socklen_t type available]),[])
AC_CHECK_FUNC(connect,,[AC_CHECK_LIB(socket,connect)])
AC_CHECK_FUNC(gethostbyname,,[AC_CHECK_LIB(nsl,gethostbyname)])
AC_CHECK_FUNC(inet_pton,AC_DEFINE(HAVE_INET_PTON,1,[Have inet_pton()]))
AC_CHECK_FUNC(inet_ntop,AC_DEFINE(HAVE_INET_NTOP,1,[Have inet_ntop()]))
AC_CHECK_FUNC(regcomp,,AC_MSG_ERROR([can't find regcomp()]))

# Standard types
AC_CHECK_TYPE(intptr_t,,[AC_MSG_ERROR([Type intptr_t not available])])

# Check for sockaddr_storage
AC_CHECK_TYPE(struct sockaddr_storage,
	[
		AC_DEFINE(HAVE_SOCKADDR_STORAGE,1,[Have struct sockaddr_storage])

		# Check for IPv6
		AC_MSG_CHECKING([for IPv6 support])
		AC_TRY_COMPILE([
				#include <netinet/in.h>
			],[
				struct sockaddr_in6 ipv6_addr; /* check for IPv6 */
			],[
				AC_MSG_RESULT([yes])
				AC_DEFINE(HAVE_IPV6,1,[IPv6 support])
				IPV6='yes'
			],[
				AC_MSG_RESULT([no])
			])
	],[],[
		#include <netinet/in.h>
	])

# zlib support
AC_CHECK_LIB(z,deflate,ZLIB=yes,ZLIB=no)
if test "x$ZLIB" = "xyes" ; then
	LIBS="$LIBS -lz"
	AC_DEFINE(HAVE_ZLIB, 1, [We have zlib])
fi

# Boehm-Demer-Weiss GC
AC_ARG_WITH(gc,[  --with-gc=<path>        Prefix of your GC library installation],[LDFLAGS="$LDFLAGS -L$withval/lib";CXXFLAGS="$CXXFLAGS -I$withval/include"])
AC_CHECK_LIB(dl,dlopen,[LIBS="$LIBS -ldl"])
AC_CHECK_LIB(gc,GC_malloc,,[AC_MSG_ERROR([Could not find suitable libgc])])

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
awemud
data/admin.ply
data/admin.acct
data/time
data/weather
tools/replace.sh
])

chmod +x awemud

echo
echo "AweMUD NG - The Next Generation of the AwesomePlay MUD"
echo "Copyright (C) 2000,2001,2002,2003,2004  AwesomePlay Productions, Inc."
echo "See COPYING for license details."

echo
echo "  Version   ... $PACKAGE_VERSION"
echo "  Install   ... ${prefix}"
echo "  Documents ... ${DOCS}"
if test "x$IPV6" = "xyes" ; then
	echo "   -- IPv6 Enabled --"
fi
if test "x$XMLRPC" = "xyes" ; then
	echo "   -- XML-RPC Enabled --"
fi
if test "x$ZLIB" = "xyes" ; then
	echo "   -- MCCP2 Enabled -- "
fi
if test "x$SENDMAIL" = "xyes" ; then
	echo "   -- Mail Enabled -- "
fi

echo
echo "Now type \"make\" to build AweMUD NG"
echo
