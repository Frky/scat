/* Conversion from UTF-32 to legacy encodings.
   Copyright (C) 2002, 2006-2007, 2009-2017 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>.  */

#include <config.h>

/* Specification.  */
#include "uniconv.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "striconveha.h"
#include "unistr.h"

#define SIZEOF(array) (sizeof (array) / sizeof (array[0]))

/* Name of UTF-32 or UCS-4 encoding with machine dependent endianness and
   alignment.  */
#if defined _LIBICONV_VERSION
# define UTF32_NAME "UCS-4-INTERNAL"
#elif ((__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 2)) && !defined __UCLIBC__
# define UTF32_NAME "WCHAR_T"
#endif

#define FUNC u32_conv_to_encoding
#define UNIT uint32_t
#define U_TO_U8 u32_to_u8
#define U_MBLEN u32_mblen
#if defined UTF32_NAME
# define UTF_NAME UTF32_NAME
# define HAVE_UTF_NAME 1
#endif
#include "u-conv-to-enc.h"
