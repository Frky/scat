/* Test of exp() function.
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

#include <math.h>

#include "signature.h"
SIGNATURE_CHECK (exp, double, (double));

#include <float.h>

#include "macros.h"

#define DOUBLE double
#define L_(literal) literal
#define MANT_DIG DBL_MANT_DIG
#define EXP exp
#define RANDOM randomd
#include "test-exp.h"

int
main ()
{
  /* A particular value.  */
  x = 0.6;
  y = exp (x);
  ASSERT (y >= 1.822118800 && y <= 1.822118801);

  test_function ();

  return 0;
}
