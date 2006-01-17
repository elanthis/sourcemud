/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef __TYPES__
#define __TYPES__

// STANDARD TYPES
typedef unsigned char		uchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;

// SPECIFIC SIZE TYPES
typedef char				int8;
typedef unsigned char		uint8;

typedef short int			int16;
typedef unsigned short int	uint16;

typedef int					int32;
typedef unsigned int		uint32;

typedef long long			int64;
typedef unsigned long long	uint64;

#ifndef NULL
#define NULL ((void *)0L)
#endif

#endif
