/* Test whether case matters for an UTF-16 string.
   Copyright (C) 2009-2017 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2009.

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
#include "unicase.h"

#include <errno.h>
#include <stdlib.h>

#include "uninorm.h"
#include "unistr.h"

#define FUNC u16_is_cased
#define UNIT uint16_t
#define U_NORMALIZE u16_normalize
#define U_TOUPPER u16_toupper
#define U_TOLOWER u16_tolower
#define U_TOTITLE u16_totitle
#define U_CMP u16_cmp
#include "u-is-cased.h"
