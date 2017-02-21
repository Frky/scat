/* Look at first character in UTF-16 string.
   Copyright (C) 1999-2000, 2002, 2006-2007, 2009-2017 Free Software
   Foundation, Inc.
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

int
u16_strmbtouc (ucs4_t *puc, const uint16_t *s)
{
  /* Keep in sync with unistr.h and u16-mbtouc-aux.c.  */
  uint16_t c = *s;

  if (c < 0xd800 || c >= 0xe000)
    {
      *puc = c;
      return (c != 0 ? 1 : 0);
    }
  if (c < 0xdc00)
    {
      if (s[1] >= 0xdc00 && s[1] < 0xe000)
        {
          *puc = 0x10000 + ((c - 0xd800) << 10) + (s[1] - 0xdc00);
          return 2;
        }
    }
  /* invalid or incomplete multibyte character */
  return -1;
}
