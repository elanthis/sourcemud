/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_TYPES_H
#define SOURCEMUD_COMMON_TYPES_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_STDINT_H) || defined(HAVE_INTTYPES_H)
#if defined(HAVE_STDINT_H)
#include "stdint.h"
#endif
#if defined(HAVE_INTTYPES_H)
#include "inttypes.h"
#endif

// STANDARD TYPES
typedef unsigned char		uchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;

// SPECIFIC SIZE TYPES
typedef int8_t				int8;
typedef uint8_t				uint8;

typedef int16_t				int16;
typedef uint16_t			uint16;

typedef int32_t				int32;
typedef uint32_t			uint32;

typedef int64_t				int64;
typedef uint64_t			uint64;

#else // defined(HAVE_STDINT_H) || defined(HAVE_INTTYPES_H)

// WARNING: only valid for ILP32 systems

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

#endif

#ifndef NULL
#define NULL ((void *)0L)
#endif

#endif
