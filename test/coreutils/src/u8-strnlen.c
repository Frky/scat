/* Determine bounded length of UTF-8 string.
   Copyright (C) 1999, 2002, 2006, 2009-2017 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2002.

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

/* Ensure strnlen() gets declared.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif

#include <config.h>

/* Specification.  */
#include "unistr.h"

#if __GLIBC__ >= 2 || defined __UCLIBC__

# include <string.h>

size_t
u8_strnlen (const uint8_t *s, size_t maxlen)
{
  return strnlen ((const char *) s, maxlen);
}

#else

# define FUNC u8_strnlen
# define UNIT uint8_t
# include "u-strnlen.h"

#endif
