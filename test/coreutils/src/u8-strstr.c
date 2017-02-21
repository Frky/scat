/* Substring test for UTF-8 strings.
   Copyright (C) 1999, 2002, 2006, 2010-2017 Free Software Foundation, Inc.
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

#include <config.h>

/* Specification.  */
#include "unistr.h"

#include <string.h>

/* FIXME: Maybe walking the string via u8_mblen is a win?  */

#define FUNC u8_strstr
#define UNIT uint8_t
#define U_STRCHR u8_strchr
#define U_STRMBTOUC u8_strmbtouc
#define UNIT_IS_UINT8_T 1
#include "u-strstr.h"
