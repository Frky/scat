/* Formatted output to a file descriptor.
   Copyright (C) 2009-2017 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include <stdio.h>

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>

#include "full-write.h"
#include "vasnprintf.h"

int
dprintf (int fd, const char *format, ...)
{
  char buf[2000];
  char *output;
  size_t len;
  size_t lenbuf = sizeof (buf);
  va_list args;

  va_start (args, format);
  output = vasnprintf (buf, &lenbuf, format, args);
  len = lenbuf;
  va_end (args);

  if (!output)
    return -1;

  if (full_write (fd, output, len) < len)
    {
      if (output != buf)
        {
          int saved_errno = errno;
          free (output);
          errno = saved_errno;
        }
      return -1;
    }

  if (output != buf)
    free (output);

  if (len > INT_MAX)
    {
      errno = EOVERFLOW;
      return -1;
    }

  return len;
}
