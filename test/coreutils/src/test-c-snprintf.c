/* Test of c_snprintf() function.
   Copyright (C) 2011-2017 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

#include "c-snprintf.h"

#include <locale.h>
#include <stdio.h>
#include <string.h>

#include "macros.h"

int
main (int argc, char *argv[])
{
  /* configure should already have checked that the locale is supported.  */
  if (setlocale (LC_ALL, "") == NULL)
    return 1;

  /* Test behaviour of snprintf() as a "control group".
     (We should be running in a locale where ',' is the decimal point.) */
  {
    char s[16];

    snprintf (s, sizeof s, "%#.0f", 1.0);
    if (!strcmp (s, "1."))
      {
        /* Skip the test, since we're not in a useful locale for testing. */
        return 77;
      }
    ASSERT (!strcmp (s, "1,"));
  }

  /* Test behaviour of c_snprintf().
     It should always use '.' as the decimal point. */
  {
    char s[16];

    c_snprintf (s, sizeof s, "%#.0f", 1.0);
    ASSERT (!strcmp (s, "1."));
  }

  return 0;
}
