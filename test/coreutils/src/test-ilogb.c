/* Test of ilogb() function.
   Copyright (C) 2012-2017 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2012.  */

#include <config.h>

#include <math.h>

#include "signature.h"
SIGNATURE_CHECK (ilogb, int, (double));

#include <float.h>
#include <limits.h>

#include "minus-zero.h"
#include "infinity.h"
#include "nan.h"
#include "macros.h"

#undef INFINITY
#undef NAN

#define DOUBLE double
/* The use of 'volatile' guarantees that excess precision bits are dropped
   when dealing with denormalized numbers.  It is necessary on x86 systems
   where double-floats are not IEEE compliant by default, to avoid that the
   results become platform and compiler option dependent.  'volatile' is a
   portable alternative to gcc's -ffloat-store option.  */
#define VOLATILE volatile
#define INFINITY Infinityd ()
#define NAN NaNd ()
#define L_(literal) literal
#define MINUS_ZERO minus_zerod
#define MAX_EXP DBL_MAX_EXP
#define MIN_EXP DBL_MIN_EXP
#define MIN_NORMAL_EXP DBL_MIN_EXP
#define ILOGB ilogb
#define RANDOM randomd
#include "test-ilogb.h"

int
main ()
{
  test_function ();

  return 0;
}
