// SOURCE MUD CONFIGURATION HEADER
#include "config.h"

// C++ Language Support
#include <typeinfo>

// C++ STL
#include <algorithm>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

// C++ IOStreams
#include <iostream>
#include <fstream>
#include <sstream>

// C++ TR1
#include <tr1/memory>

// C++ ANSI C Wrappers
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <climits>

// POSIX
#include <arpa/inet.h>
#include <sys/ioctl.h>
#if defined(HAVE_POLL)
#	include <sys/poll.h>
#endif
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <grp.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

// Standard C (C99/optional)
#if defined(HAVE_STDINT_H) || defined(HAVE_INTTYPES_H)
#	if defined(HAVE_STDINT_H)
#		include <stdint.h>
#	endif
#	if defined(HAVE_INTTYPES_H)
#		include <inttypes.h>
#	endif
#else
#	error "stdint.h or inttypes.h required"
#endif

// Libraries
#ifdef HAVE_LIBZ
#	include <zlib.h>
#endif // HAVE_LIBZ

// common.h has been define
#define COMMON_H 1
