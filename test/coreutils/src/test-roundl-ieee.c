/* Test of rounding to nearest, breaking ties away from zero.
   Copyright (C) 2010-2017 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

#include <math.h>

#include "fpucw.h"
#include "isnanl-nolibm.h"
#include "minus-zero.h"
#include "infinity.h"
#include "nan.h"
#include "macros.h"

int
main ()
{
  DECL_LONG_DOUBLE_ROUNDING

  BEGIN_LONG_DOUBLE_ROUNDING ();

  /* See IEEE 754, section 6.3:
       "the sign of the result of the round floating-point number to
        integral value operation is the sign of the operand. These rules
        shall apply even when operands or results are zero or infinite."  */

  /* Zero.  */
  ASSERT (!signbit (roundl (0.0L)));
  ASSERT (!!signbit (roundl (minus_zerol)) == !!signbit (minus_zerol));
  /* Positive numbers.  */
  ASSERT (!signbit (roundl (0.3L)));
  ASSERT (!signbit (roundl (0.7L)));
  /* Negative numbers.  */
  ASSERT (!!signbit (roundl (-0.3L)) == !!signbit (minus_zerol));
  ASSERT (!!signbit (roundl (-0.7L)) == !!signbit (minus_zerol));

  /* [MX] shaded specification in POSIX.  */

  /* NaN.  */
  ASSERT (isnanl (roundl (NaNl ())));
  /* Infinity.  */
  ASSERT (roundl (Infinityl ()) == Infinityl ());
  ASSERT (roundl (- Infinityl ()) == - Infinityl ());

  return 0;
}
