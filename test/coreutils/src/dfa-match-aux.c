/* Auxiliary program to test a DFA code path that cannot be triggered
   by grep or gawk.
   Copyright 2014-2017 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include <config.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <dfa.h>
#include <localeinfo.h>

#include "getprogname.h"

_Noreturn void
dfaerror (char const *mesg)
{
  printf ("dfaerror: %s\n", mesg);
  exit (EXIT_FAILURE);
}

_Noreturn void
dfawarn (char const *mesg)
{
  printf ("dfawarn: %s\n", mesg);
  exit (EXIT_FAILURE);
}

int
main (int argc, char **argv)
{
  struct dfa *dfa;
  char *beg, *end, *p;
  int allow_nl;
  struct localeinfo localeinfo;

  if (argc < 3)
    exit (EXIT_FAILURE);

  setlocale (LC_ALL, "");
  init_localeinfo (&localeinfo);

  dfa = dfaalloc ();
  dfasyntax (dfa, &localeinfo, RE_SYNTAX_GREP | RE_NO_EMPTY_RANGES, 0);
  dfacomp (argv[1], strlen (argv[1]), dfa, 0);

  beg = argv[2];
  end = argv[2] + strlen (argv[2]);
  allow_nl = argc > 3 && atoi (argv[3]);

  p = dfaexec (dfa, beg, end, allow_nl, NULL, NULL);

  if (p != NULL)
    printf ("%zd\n", p - beg);

  exit (EXIT_SUCCESS);
}
