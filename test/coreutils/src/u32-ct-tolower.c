/* Lowercase mapping for UTF-32 substrings (locale dependent).
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

#include <stddef.h>

#include "unicasemap.h"
#include "special-casing.h"

uint32_t *
u32_ct_tolower (const uint32_t *s, size_t n,
                casing_prefix_context_t prefix_context,
                casing_suffix_context_t suffix_context,
                const char *iso639_language,
                uninorm_t nf,
                uint32_t *resultbuf, size_t *lengthp)
{
  return u32_casemap (s, n, prefix_context, suffix_context, iso639_language,
                      uc_tolower, offsetof (struct special_casing_rule, lower[0]),
                      nf,
                      resultbuf, lengthp);
}
