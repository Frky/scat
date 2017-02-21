/* Invoke tmpfile, but avoid some glitches.
   Copyright (C) 2006, 2009-2017 Free Software Foundation, Inc.

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

/* Written by Eric Blake, based on ideas from Paul Eggert.  */

#include <config.h>

#include "stdio-safer.h"

#include <errno.h>
#include <unistd.h>
#include "unistd-safer.h"

#include "binary-io.h"

/* Like tmpfile, but do not return stdin, stdout, or stderr.

   Remember that tmpfile can leave files behind if your program calls _exit,
   so this function should not be mixed with the close_stdout module.  */

FILE *
tmpfile_safer (void)
{
  FILE *fp = tmpfile ();

  if (fp)
    {
      int fd = fileno (fp);

      if (0 <= fd && fd <= STDERR_FILENO)
        {
          int f = dup_safer (fd);

          if (f < 0)
            {
              int e = errno;
              fclose (fp);
              errno = e;
              return NULL;
            }

          /* Keep the temporary file in binary mode, on platforms
             where that matters.  */
          if (fclose (fp) != 0
              || ! (fp = fdopen (f, O_BINARY ? "wb+" : "w+")))
            {
              int e = errno;
              close (f);
              errno = e;
              return NULL;
            }
        }
    }

  return fp;
}
