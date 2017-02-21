/* Test of freadable() function.
   Copyright (C) 2007-2017 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

#include <config.h>

/* None of the files accessed by this test are large, so disable the
   fseek link warning if we are not using the gnulib fseek module.  */
#define _GL_NO_LARGE_FILES
#include "freadable.h"

#include <stdio.h>

#include "macros.h"

#define TESTFILE "t-freadable.tmp"

int
main ()
{
  FILE *fp;

  /* Create a file with some contents.  */
  fp = fopen (TESTFILE, "w");
  if (fp == NULL)
    goto skip;
  ASSERT (!freadable (fp));
  if (fwrite ("foobarsh", 1, 8, fp) < 8)
    goto skip;
  ASSERT (!freadable (fp));
  if (fclose (fp))
    goto skip;

  /* Open it in read-only mode.  */
  fp = fopen (TESTFILE, "r");
  if (fp == NULL)
    goto skip;
  ASSERT (freadable (fp));
  if (fgetc (fp) != 'f')
    goto skip;
  ASSERT (freadable (fp));
  if (fseek (fp, 2, SEEK_CUR))
    goto skip;
  ASSERT (freadable (fp));
  if (fgetc (fp) != 'b')
    goto skip;
  ASSERT (freadable (fp));
  fflush (fp);
  ASSERT (freadable (fp));
  if (fgetc (fp) != 'a')
    goto skip;
  ASSERT (freadable (fp));
  if (fseek (fp, 0, SEEK_END))
    goto skip;
  ASSERT (freadable (fp));
  if (fclose (fp))
    goto skip;

  /* Open it in read-write mode.  */
  fp = fopen (TESTFILE, "r+");
  if (fp == NULL)
    goto skip;
  ASSERT (freadable (fp));
  if (fgetc (fp) != 'f')
    goto skip;
  ASSERT (freadable (fp));
  if (fseek (fp, 2, SEEK_CUR))
    goto skip;
  ASSERT (freadable (fp));
  if (fgetc (fp) != 'b')
    goto skip;
  ASSERT (freadable (fp));
  fflush (fp);
  ASSERT (freadable (fp));
  if (fgetc (fp) != 'a')
    goto skip;
  ASSERT (freadable (fp));
  if (fputc ('z', fp) != 'z')
    goto skip;
  ASSERT (freadable (fp));
  if (fseek (fp, 0, SEEK_END))
    goto skip;
  ASSERT (freadable (fp));
  if (fclose (fp))
    goto skip;

  /* Open it in append mode.  */
  fp = fopen (TESTFILE, "a");
  if (fp == NULL)
    goto skip;
  ASSERT (!freadable (fp));
  if (fwrite ("bla", 1, 3, fp) < 3)
    goto skip;
  ASSERT (!freadable (fp));
  if (fclose (fp))
    goto skip;

  return 0;

 skip:
  fprintf (stderr, "Skipping test: file operations failed.\n");
  return 77;
}
