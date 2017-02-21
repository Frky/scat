/* Test of log1pf() function.
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
SIGNATURE_CHECK (log1pf, float, (float));

#include <float.h>

#include "macros.h"

#define DOUBLE float
#define HUGEVAL HUGE_VALF
#define L_(literal) literal##f
#define MANT_DIG FLT_MANT_DIG
#define LOG1P log1pf
#define RANDOM randomf
#include "test-log1p.h"

int
main ()
{
  /* A particular value.  */
  x = 0.6f;
  y = log1pf (x);
  ASSERT (y >= 0.4700036f && y <= 0.4700037f);

  test_function ();

  return 0;
}
