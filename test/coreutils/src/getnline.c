/* getnline - Read a line from a stream, with bounded memory allocation.

   Copyright (C) 2003-2004, 2006, 2009-2017 Free Software Foundation, Inc.

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

/* Specification.  */
#include "getnline.h"

#include "getndelim2.h"

ssize_t
getndelim (char **lineptr, size_t *linesize, size_t nmax,
           int delimiter, FILE *stream)
{
  return getndelim2 (lineptr, linesize, 0, nmax, delimiter, EOF, stream);
}

ssize_t
getnline (char **lineptr, size_t *linesize, size_t nmax, FILE *stream)
{
  return getndelim (lineptr, linesize, nmax, '\n', stream);
}
