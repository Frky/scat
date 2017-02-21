/* Test of terminating the current process.
   Copyright (C) 2010-2017 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2010.  */

#include <config.h>

#include <stdlib.h>

/* But did he ever return?  No he never returned,
   And his fate is still unlearned ... */
static _Noreturn void MTA (int);

static _Noreturn void
Charlie (int n)
{
  MTA (n - 1);
}

static void
MTA (int n)
{
  if (n < 0)
    _Exit (81);
  Charlie (n - 1);
}

int
main (int argc, char **argv)
{
  MTA (argc + !!argv);
}
