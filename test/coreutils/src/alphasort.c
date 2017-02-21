/* Copyright (C) 1992, 1997-1998, 2009-2017 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

#include <dirent.h>

#include <string.h>

int
#ifndef __KLIBC__
alphasort (const struct dirent **a, const struct dirent **b)
{
  return strcoll ((*a)->d_name, (*b)->d_name);
}
#else
/* On OS/2 kLIBC, the compare function declaration of scandir() is different
   from POSIX. See <http://trac.netlabs.org/libc/browser/branches/libc-0.6/src/emx/include/dirent.h#L141>.  */
alphasort (const void *a, const void *b)
{
  return strcoll ((*(const struct dirent **)a)->d_name,
                  (*(const struct dirent **)b)->d_name);
}
#endif
